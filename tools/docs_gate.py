#!/usr/bin/env python3
"""docs_gate.py — one-shot documentation consistency gate for the D2VP-QoL repo.

Round-agnostic: the round under test is derived from `#define D2VP_ROUND` in
Redivivus/d2_boot.h (or passed with --round). Checks:

  1. round identity    boot define <-> handoff rev header <-> handoff §1
                       self-ID <-> CHANGELOG top bullet <-> ROUND-SUMMARY title
  2. handoff 16 KB gate  (default 16384 bytes, --gate to override)
  3. smoke sync        checklist <-> template contract for this round
                       (delegates to smoke_sync.check, incl. field-note rules)
  4. item numbering    duplicate checkbox item numbers in either file
  5. options page      gaVanillaPlusRows: declared size == entries == unique;
                       VPP_ROW_COUNT == VPP_ROW_BACK == entries;
                       entries <= 2 * VPP_ROWS_PER_COL;
                       geometry arithmetic (band/back/box margins) + 480p fit;
                       ini template covers every row key
  6. feature count     FEATURES "N features ship" == row count;
                       handoff §2 "N `[features]` toggles" == row count
  7. stale rounds      no foreign D2VP_ROUND literal in boot; no older
                       self-ID in handoff §1; per-doc older-round census (info)
  8. VP markers        VP-BEGIN/VP-END pairing on git-changed files
  9. C balance         brace/paren balance on git-changed .cpp/.h files
 10. overlay zip       if download/d2vp-r<N>-overlay.zip exists: entry set ==
                       git-changed set and every entry byte-identical to the
                       working tree (info if absent — run overlay.py verify)

Exit 0 = all green (warnings allowed), 1 = at least one FAIL.
"""
import argparse, os, re, subprocess, sys, zipfile, hashlib

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import smoke_sync as ss  # section/item parser + sync contract

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
# output dir defaults to <repo-parent>/download; --download overrides
DEFAULT_DOWNLOAD = os.path.join(os.path.dirname(DEFAULT_REPO), 'download')
WORD = {'zero': 0, 'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5, 'six': 6,
        'seven': 7, 'eight': 8, 'nine': 9, 'ten': 10, 'eleven': 11, 'twelve': 12,
        'thirteen': 13, 'fourteen': 14, 'fifteen': 15, 'sixteen': 16,
        'seventeen': 17, 'eighteen': 18, 'nineteen': 19, 'twenty': 20,
        'a hundred': 100, 'hundred': 100}

fail = []
warn_list = []


def chk(name, cond, warn_only=False):
    if cond:
        print('PASS ' + name)
    else:
        print(('WARN ' if warn_only else 'FAIL ') + name)
        (warn_list if warn_only else fail).append(name)


def load(repo, rel):
    with open(os.path.join(repo, rel), encoding='utf8') as f:
        return f.read()


def git_changed(repo):
    out = subprocess.run(['git', 'status', '--porcelain'], cwd=repo,
                         capture_output=True, text=True).stdout
    return sorted(line[3:].strip() for line in out.splitlines() if line.strip())


def word2num(s):
    s = s.lower().strip()
    if s in WORD:
        return WORD[s]
    try:
        return int(s)
    except ValueError:
        return None


def parse_int(txt, pat):
    m = re.search(pat, txt)
    return int(m.group(1), 0) if m else None


def check_round_identity(repo, n):
    boot = load(repo, 'Redivivus/d2_boot.h')
    chk('boot: exactly one D2VP_ROUND define',
        len(re.findall(r'#define D2VP_ROUND "\d+"', boot)) == 1)
    chk(f'boot: D2VP_ROUND "{n}"', f'#define D2VP_ROUND "{n}"' in boot)
    chk('boot: D2VP_BUILD_TAG composes from D2VP_ROUND',
        '#define D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND' in boot)

    handoff = load(repo, 'handoff.md')
    m = re.match(r'# main branch — handoff \(rev (\d+), (\d{4}-\d{2}-\d{2}) — \*\*R(\d+)', handoff)
    chk('handoff: rev header parses (rev, date, R-round)', m is not None)
    if m:
        chk(f'handoff: header round is R{n}', int(m.group(3)) == n)
        print(f'      (handoff rev {m.group(1)}, dated {m.group(2)})')
    s1 = handoff[handoff.find('## 1.'):handoff.find('## 2.')]
    chk(f'handoff §1: self-ID Vanilla+0.0.8-R{n}',
        f'Vanilla+0.0.8-R{n}' in s1)

    changelog = load(repo, 'CHANGELOG.md')
    top = re.search(r'^- \*\*R(\d+)[ —-]', changelog, re.M)
    chk(f'CHANGELOG: top feature bullet is R{n}', top is not None and int(top.group(1)) == n)

    summary = load(repo, 'ROUND-SUMMARY.md')
    chk(f'ROUND-SUMMARY: title round R{n}',
        bool(re.match(r'# Round summary — R%d\b' % n, summary)))
    return handoff, m


def check_options_page(repo, n):
    opts = load(repo, 'Source/D2Client/UI/VanillaPlusOptions.cpp')
    core = load(repo, 'Source/D2Client/UI/VanillaPlusCore.cpp')

    declared = parse_int(opts, r'gaVanillaPlusRows\[(\d+)\]')
    entries = re.findall(r'\{ "([a-z_0-9]+)", L"', opts)
    chk(f'options: gaVanillaPlusRows[{declared}] == {len(entries)} entries',
        declared == len(entries))
    chk('options: row keys unique', len(set(entries)) == len(entries))
    row_count = parse_int(opts, r'VPP_ROW_COUNT = (\d+)')
    row_back = parse_int(opts, r'VPP_ROW_BACK = (\d+)')
    chk('options: VPP_ROW_COUNT == entries', row_count == len(entries))
    chk('options: VPP_ROW_BACK == entries', row_back == len(entries))
    rpc = parse_int(opts, r'VPP_ROWS_PER_COL = (\d+)')
    chk(f'options: entries fit two columns of {rpc}',
        len(entries) <= 2 * rpc and len(entries) > rpc)

    # geometry arithmetic
    first = parse_int(opts, r'VPPAGE_FIRST_ROW_Y = (0x[0-9a-fA-F]+|\d+)')
    pitch = parse_int(opts, r'VPPAGE_ROW_PITCH = (0x[0-9a-fA-F]+|\d+)')
    rowh = parse_int(opts, r'VPPAGE_ROW_HEIGHT = (0x[0-9a-fA-F]+|\d+)')
    back = parse_int(opts, r'VPPAGE_BACK_ROW_Y = (0x[0-9a-fA-F]+|\d+)')
    box = parse_int(opts, r'VPPAGE_BOX_HEIGHT = (0x[0-9a-fA-F]+|\d+)')
    ok_geo = None
    if None not in (first, pitch, rowh, back, box, rpc):
        # layout model (R144/R147): bands at first + i*pitch (height rowh);
        # the Previous-Menu row starts one full row-height BELOW the last
        # band's bottom edge; the box adds the same margin below it.
        band_bottom = first + (rpc - 1) * pitch + rowh
        exp_back = band_bottom + rowh
        margin = box - (back + rowh)
        ok_geo = (back == exp_back) and (margin > 0)
        top480 = (480 - box) / 2
        chk(f'options: back row {back:#x} == last-band bottom {band_bottom:#x} + rowh',
            back == exp_back)
        chk(f'options: box bottom margin positive ({margin} px)', margin > 0)
        chk(f'options: box {box} px fits 640x480 (top margin {top480:.0f} px)',
            top480 >= 0)
    else:
        chk('options: geometry constants all present', False)

    # ini template covers every row key
    tpl_keys = set(re.findall(r'"([a-z_0-9]+)=\d', core))
    missing = set(entries) - tpl_keys
    chk('options: ini template covers every row key', not missing)
    if tpl_keys - set(entries):
        warn_list.append('ini template carries extra keys (ini-only by design?): '
                         + ', '.join(sorted(tpl_keys - set(entries))))
        print('WARN  options: template has extra keys (ini-only by design?): '
              + ', '.join(sorted(tpl_keys - set(entries))))
    return len(entries)


def check_feature_counts(repo, n, row_count):
    features = load(repo, 'FEATURES.md')
    m = None
    for m in re.finditer(r'\*\*([A-Za-z ]+?) features ship', features):
        pass
    if m:
        chk(f'FEATURES: "{m.group(1)} features ship" == {row_count} rows',
            word2num(m.group(1)) == row_count)
    else:
        chk('FEATURES: "N features ship" statement present', False)

    handoff = load(repo, 'handoff.md')
    s2 = handoff[handoff.find('## 2.'):handoff.find('## 3')]
    m2 = re.search(r'(\w+|\d+) `\[features\]` toggles', s2)
    chk('handoff §2: "[features] toggles" count == row count',
        m2 is not None and word2num(m2.group(1)) == row_count)


def check_stale_rounds(repo, n, handoff):
    boot = load(repo, 'Redivivus/d2_boot.h')
    chk('boot: no foreign round literal in a D2VP_ROUND define',
        not re.search(r'#define D2VP_ROUND "(?!%d")\d+"' % n, boot))
    s1 = handoff[handoff.find('## 1.'):handoff.find('## 2.')]
    old = re.findall(r'Vanilla\+0\.0\.8-R(\d+)', s1)
    chk('handoff §1: no older self-ID', all(int(x) == n for x in old))

    print('      older-round census (info):')
    for rel in ('CHANGELOG.md', 'README.md', 'FEATURES.md', 'SMOKE-CHECKLIST.md',
                'ROUND-SUMMARY.md', 'handoff.md'):
        txt = load(repo, rel)
        cnt = len(re.findall(r'R%d\b' % (n - 1), txt)) if n > 1 else 0
        print(f'      {rel}: {cnt} R{n - 1} mention(s)')


def _strip_c_dialect(txt):
    # [R155] single-pass state machine: code, // comment, /* comment */,
    # "string", 'char'. Escapes honored inside strings/chars; comments and
    # literal bodies collapse to nothing/empties so brace/paren counting sees
    # only real code. Replaces the R149-era ordered-regex chain (see the
    # check_markers_and_balance note).
    out = []
    i, n = 0, len(txt)
    state = 'code'
    while i < n:
        c = txt[i]
        nxt = txt[i + 1] if i + 1 < n else ''
        if state == 'code':
            if c == '/' and nxt == '/':
                state = 'line'; i += 2; continue
            if c == '/' and nxt == '*':
                state = 'block'; i += 2; continue
            if c == '"':
                state = 'str'; out.append('""'); i += 1; continue
            if c == "'":
                state = 'chr'; out.append("''"); i += 1; continue
            out.append(c); i += 1
        elif state == 'line':
            if c == '\n':
                state = 'code'; out.append('\n')
            i += 1
        elif state == 'block':
            if c == '*' and nxt == '/':
                state = 'code'; i += 2; continue
            i += 1
        elif state == 'str':
            if c == '\\':
                i += 2; continue
            if c == '"' or c == '\n':
                state = 'code'
            i += 1
        else:  # chr
            if c == '\\':
                i += 2; continue
            if c == "'" or c == '\n':
                state = 'code'
            i += 1
    return ''.join(out)


def check_markers_and_balance(repo, changed):
    # [R149] markers pair as a MULTISET (the tree's established idiom nests
    # VP blocks — Path.cpp's case guards share spans — so file-order list
    # equality false-positived); balance strips comments and string/char
    # literals first (retail code legally contains lone parens inside
    # strings — Inv.cpp's strcat(..., ")") UTF-8 builders).
    # [R155] the strip is a small state machine now, not four ordered regexes:
    # the regex chain ran line-comments BEFORE block-comments, so a URL with
    # a `//` inside a /* ... */ block (D2GFX.cpp's {@url https://...} doc
    # comment) truncated the line and corrupted the brace count, and a `/*`
    # inside a // comment (Select.cpp's "/*pX/*pY" doc) would over-eat the
    # other way. A real lexer's order — context, not sequence — has neither
    # failure mode.
    for rel in changed:
        if not (rel.endswith('.cpp') or rel.endswith('.h')):
            continue
        p = os.path.join(repo, rel)
        if not os.path.exists(p):
            continue
        txt = open(p, encoding='utf8').read()
        begins = re.findall(r'VP-BEGIN \[([^\]]+)\]', txt)
        ends = re.findall(r'VP-END \[([^\]]+)\]', txt)
        if begins or ends:
            chk(f'markers: {rel} VP-BEGIN/VP-END paired', sorted(begins) == sorted(ends))
        stripped = _strip_c_dialect(txt)
        chk(f'balance: {rel} braces', stripped.count('{') == stripped.count('}'))
        chk(f'balance: {rel} parens', stripped.count('(') == stripped.count(')'))


def check_zip(repo, n, changed, download_dir):
    zp = os.path.join(download_dir, f'd2vp-r{n}-overlay.zip')
    if not os.path.exists(zp):
        print(f'INFO  no d2vp-r{n}-overlay.zip yet (build with overlay.py pack --round {n})')
        return
    z = zipfile.ZipFile(zp)
    names = sorted(z.namelist())
    chk(f'zip: entry set == git-changed set ({len(names)})', names == sorted(changed))
    bad = []
    for rel in names:
        p = os.path.join(repo, rel)
        if not os.path.exists(p):
            bad.append(rel + ' (gone from tree)')
            continue
        zb, tb = z.read(rel), open(p, 'rb').read()
        if hashlib.sha256(zb).hexdigest() != hashlib.sha256(tb).hexdigest():
            bad.append(rel)
    chk('zip: all entries byte-identical to the working tree', not bad)
    if bad:
        print('      stale entries: ' + ', '.join(bad))


def main():
    ap = argparse.ArgumentParser(description='D2VP-QoL docs consistency gate',
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--round', type=int, help='round under test (default: D2VP_ROUND)')
    ap.add_argument('--repo', default=DEFAULT_REPO)
    ap.add_argument('--download', default=DEFAULT_DOWNLOAD)
    ap.add_argument('--gate', type=int, default=16384, help='handoff byte cap')
    a = ap.parse_args()
    if not os.path.isdir(a.repo):
        print(f'FAIL: repo not found: {a.repo}')
        return 1

    boot = load(a.repo, 'Redivivus/d2_boot.h')
    mb = re.search(r'#define D2VP_ROUND "(\d+)"', boot)
    n = a.round or (int(mb.group(1)) if mb else None)
    if not n:
        print('FAIL: no round derivable')
        return 1
    print(f'== docs gate — round R{n} ==')

    handoff, hm = check_round_identity(a.repo, n)
    size = len(handoff.encode('utf8'))
    chk(f'handoff size {size} <= {a.gate} ({a.gate - size} headroom)', size <= a.gate)

    changed = git_changed(a.repo)
    row_count = check_options_page(a.repo, n)
    check_feature_counts(a.repo, n, row_count)
    check_stale_rounds(a.repo, n, handoff)
    check_markers_and_balance(a.repo, changed)
    check_zip(a.repo, n, changed, a.download)

    # smoke sync contract (delegated)
    print('== smoke sync contract ==')
    rc = ss.check(a.repo, n)
    if rc:
        fail.append('smoke sync contract')

    tail = f' — {len(fail)} fail(s), {len(warn_list)} warning(s)' if (fail or warn_list) else ''
    print(f'\n{"ALL PASS" if not fail else "GATE FAILED"}{tail}')
    if fail:
        print('fails: ' + '; '.join(fail))
    return 1 if fail else 0


if __name__ == '__main__':
    sys.exit(main())
