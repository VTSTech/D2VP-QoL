#!/usr/bin/env python3
"""smoke_sync.py — SMOKE-CHECKLIST.md <-> issue-template section/item sync.

The checklist (.github canonical: SMOKE-CHECKLIST.md) and the issue template
(.github/ISSUE_TEMPLATE/smoke-test.md) must carry the same CHECKS, but they are
not byte-equal as whole files: field-history notes live only in the checklist,
so untouched sections legitimately differ. Convention (R146/R147 practice):

  - a round that REWRITES a whole section syncs it verbatim into the template
  - a round that touches single ITEMS syncs those item lines verbatim
  - new sections are inserted at the matching position

Commands:
  copy --section T       copy whole section T (canonical -> template), inserting
                         it at the position of the next template section if new
  copy --item 67         copy one checkbox item line (canonical -> template)
  check [--round N]      apply the sync contract for round N (default: the
                         D2VP_ROUND of Redivivus/d2_boot.h):
                           * sections whose header OR setup line mentions R<N>
                             must be byte-identical in both files
                           * every item line mentioning R<N> must be identical
                           * a canonical-only section mentioning R<N> must
                             exist in the template
  diff                   report all differing sections/items (informational)

Exit 0 = contract holds (or copy done), 1 = mismatch/anchor problem.
"""
import argparse, re, sys, os

_HERE = os.path.dirname(os.path.abspath(__file__))
# repo root = the directory containing Redivivus/ — resolves both the in-repo
# layout (tools/..) and a sandbox workspace (scripts/ beside the clone);
# --repo overrides.
_REPO_CANDIDATES = [
    os.path.dirname(_HERE),                            # in-repo layout (tools/)
    os.path.join(os.path.dirname(_HERE), 'D2VP-QoL'),  # sandbox layout
]
DEFAULT_REPO = next((p for p in _REPO_CANDIDATES
                     if os.path.isdir(os.path.join(p, 'Redivivus'))),
                    _REPO_CANDIDATES[-1])
CK_REL = 'SMOKE-CHECKLIST.md'
TPL_REL = '.github/ISSUE_TEMPLATE/smoke-test.md'
BOOT_REL = 'Redivivus/d2_boot.h'
SEP = '\n---\n'  # section separator line used by both files


def load(repo, rel):
    with open(os.path.join(repo, rel), encoding='utf8') as f:
        return f.read()


def save(repo, rel, txt):
    with open(os.path.join(repo, rel), 'w', encoding='utf8', newline='') as f:
        f.write(txt)


def parse_sections(text):
    """-> ordered [(letter, header_line, raw_block)] where raw_block runs from
    the header line up to (not incl.) the next '## ' line or EOF, with trailing
    separator/blank lines stripped."""
    out = []
    starts = [m.start() for m in re.finditer(r'^## ', text, re.M)]
    for i, s in enumerate(starts):
        e = starts[i + 1] if i + 1 < len(starts) else len(text)
        block = text[s:e]
        block = block.rstrip()
        while block.endswith('---'):
            block = block[:-3].rstrip()
        hdr = block.split('\n', 1)[0]
        m = re.match(r'## ([A-Z0-9]+)\.', hdr)
        letter = m.group(1) if m else hdr[3:10]
        out.append((letter, hdr, block))
    return out


def get_section(text, letter):
    for l, hdr, block in parse_sections(text):
        if l == letter:
            return hdr, block
    return None, None


def items_of(text):
    """-> [(num, full_line)] in file order."""
    out = []
    for line in text.split('\n'):
        m = re.match(r'^- \[[ x]\] \*\*(\d+[a-z]?)\.', line)
        if m:
            out.append((m.group(1), line))
    return out


def current_round(repo):
    boot = load(repo, BOOT_REL)
    m = re.search(r'#define D2VP_ROUND "(\d+)"', boot)
    return int(m.group(1)) if m else None


def section_mentions_round(hdr_block, n):
    return bool(re.search(r'R%d\b' % n, hdr_block))


def copy_section(repo, letter):
    ck = load(repo, CK_REL)
    tp = load(repo, TPL_REL)
    hdr, block = get_section(ck, letter)
    if block is None:
        print(f'FAIL: section {letter} not found in {CK_REL}')
        return 1
    block = block.rstrip()

    def raw_span(text, header_line):
        """span from the header line to just before the next '## ' (or EOF) —
        includes any trailing separator, so replacement is byte-stable."""
        s = text.find(header_line)
        if s < 0:
            return None, None
        nx = re.search(r'^## ', text[s + 1:], re.M)
        e = (s + 1 + nx.start()) if nx else len(text)
        return s, e

    t_hdr, _ = get_section(tp, letter)
    if t_hdr is not None:
        s, e = raw_span(tp, t_hdr)
        if s is None:
            print(f'FAIL: section {letter} header found but span missing')
            return 1
        nxt_exists = e < len(tp)
        new_span = block + ('\n\n---\n\n' if nxt_exists else '\n')
        tp = tp[:s] + new_span + tp[e:]
        print(f'template: section {letter} replaced with the checklist copy '
              f'({len(block)} bytes)')
    else:
        # insert before the next canonical section that exists in the template
        ck_letters = [l for l, _, _ in parse_sections(ck)]
        try:
            nxt = ck_letters[ck_letters.index(letter) + 1]
        except IndexError:
            nxt = None
        n_hdr = get_section(tp, nxt)[0] if nxt else None
        if n_hdr:
            idx = tp.find('\n' + n_hdr)
            tp = tp[:idx + 1] + block + '\n\n---\n\n' + tp[idx + 1:]
            print(f'template: section {letter} inserted before {nxt}')
        else:
            tp = tp.rstrip('\n') + '\n\n' + block + '\n'
            print(f'template: section {letter} appended')
    save(repo, TPL_REL, tp)
    return 0


def copy_item(repo, num):
    ck = load(repo, CK_REL)
    tp = load(repo, TPL_REL)
    src = None
    for n, line in items_of(ck):
        if n == num:
            src = line
    if src is None:
        print(f'FAIL: item {num} not found in {CK_REL}')
        return 1
    done = False
    for n, line in items_of(tp):
        if n == num:
            tp = tp.replace(line, src, 1)
            done = True
    if not done:
        print(f'FAIL: item {num} has no counterpart in the template (use '
              f'--section copy or insert it manually)')
        return 1
    save(repo, TPL_REL, tp)
    print(f'template: item {num} replaced with the checklist copy')
    return 0


def has_field_note(line):
    """True if the checklist item carries a field-history suffix — such items
    live condensed in the template (only the round's changed phrase is carried
    over), so byte-identity is not required (R145/R147 practice)."""
    return bool(re.search(
        r'— \*\*(CONFIRMED|FIELD|VERIFIED|No regression|Glide look|owner|Owner'
        r'|20\d\d-\d\d-\d\d|R\d+)', line))


def check(repo, n):
    ck = load(repo, CK_REL)
    tp = load(repo, TPL_REL)
    bad = []
    warn = []

    ck_secs = parse_sections(ck)
    tp_secs = parse_sections(tp)
    tp_map = {l: (h, b) for l, h, b in tp_secs}

    # 1. sections whose header OR setup line mentions R<N>: whole block must be
    #    identical (round rewrote the section verbatim into the template)
    for letter, hdr, block in ck_secs:
        hdr_line = block.split('\n', 1)[0]
        setup = ''
        rest = block.split('\n\n', 1)
        if len(rest) == 2 and rest[1].startswith('Setup:'):
            setup = rest[1].split('\n', 1)[0]
        if section_mentions_round(hdr_line, n) or section_mentions_round(setup, n):
            if letter not in tp_map:
                bad.append(f'section {letter} mentions R{n} but is missing '
                           f'from the template')
            elif tp_map[letter][1] != block:
                bad.append(f'section {letter} mentions R{n} but differs from '
                           f'the checklist copy (run: copy --section {letter})')

    # 2. items mentioning R<N>:
    #    - missing from the template            -> FAIL
    #    - present, plain (no field-note tail)  -> must be byte-identical
    #    - present, with a field-note tail      -> WARN (condensed by design)
    ck_items = dict(items_of(ck))
    tp_items = dict(items_of(tp))
    for num, line in ck_items.items():
        if re.search(r'R%d\b' % n, line):
            if num not in tp_items:
                bad.append(f'item {num} mentions R{n} but is missing from the template')
            elif tp_items[num] != line:
                if has_field_note(line):
                    warn.append(f'item {num} differs — checklist copy carries a '
                                f'field-note tail (verify the R{n} phrase was '
                                f'carried into the condensed template copy)')
                else:
                    bad.append(f'item {num} mentions R{n} but differs '
                               f'(run: copy --item {num})')

    for w in warn:
        print('  WARN ' + w)
    if bad:
        print(f'CHECK R{n}: {len(bad)} problem(s), {len(warn)} warning(s)')
        for b in bad:
            print('  FAIL ' + b)
        return 1
    print(f'CHECK R{n}: sync contract holds ({len(warn)} warning(s))')
    return 0


def diff(repo):
    ck = load(repo, CK_REL)
    tp = load(repo, TPL_REL)
    ck_secs = parse_sections(ck)
    tp_map = {l: (h, b) for l, h, b in parse_sections(tp)}
    print(f'{"sec":4} {"status":10} note')
    for letter, hdr, block in ck_secs:
        if letter not in tp_map:
            print(f'{letter:4} {"ONLY-CK":10} not in template')
        elif tp_map[letter][1] == block:
            print(f'{letter:4} {"same":10}')
        else:
            print(f'{letter:4} {"differs":10} (allowed if untouched this round)')
    for letter in tp_map:
        if letter not in {l for l, _, _ in ck_secs}:
            print(f'{letter:4} {"ONLY-TP":10} template-only (e.g. Verdict)')
    ck_items = dict(items_of(ck))
    tp_items = dict(items_of(tp))
    nums_ck, nums_tp = list(ck_items), list(tp_items)
    if nums_ck != nums_tp and set(nums_ck) == set(nums_tp):
        print('NOTE: item ORDER differs between files')
    both_diff = [n for n in sorted(set(nums_ck) & set(nums_tp), key=lambda x: (int(re.match(r"\d+", x).group()), x))
                 if ck_items[n] != tp_items[n]]
    print(f'items differing: {len(both_diff)}'
          + (': ' + ', '.join(both_diff) if both_diff else ''))
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[1],
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('command', choices=['copy', 'check', 'diff'])
    ap.add_argument('--section', help='section letter for copy')
    ap.add_argument('--item', help='item number for copy')
    ap.add_argument('--round', type=int, help='round for check (default: D2VP_ROUND)')
    ap.add_argument('--repo', default=DEFAULT_REPO)
    a = ap.parse_args()

    if not os.path.isdir(a.repo):
        print(f'FAIL: repo not found: {a.repo}')
        return 1
    if a.command == 'copy':
        if not (a.section or a.item):
            print('copy needs --section LETTER or --item NUM')
            return 1
        if a.section:
            return copy_section(a.repo, a.section.upper())
        return copy_item(a.repo, a.item)
    if a.command == 'check':
        n = a.round or current_round(a.repo)
        if not n:
            print('FAIL: no round given and D2VP_ROUND not parseable')
            return 1
        return check(a.repo, n)
    return diff(a.repo)


if __name__ == '__main__':
    sys.exit(main())
