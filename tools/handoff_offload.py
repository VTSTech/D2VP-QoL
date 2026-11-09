#!/usr/bin/env python3
"""handoff_offload.py — the handoff rev-header offload (the trim-N pattern).

When a new round's header replaces the old one in handoff.md, the old header
moves verbatim into HISTORY.md as the next H-record, and handoff §8's H-index
gains the offload note. This script does the mechanical part:

  1. reads the CURRENT handoff line-1 header (parses rev + round + slug)
  2. appends H<n+1> to HISTORY.md:
       ## H<n+1>. <round> — <slug-tail> (<header date>) — verbatim handoff line
                    at the rev-<old> state (offloaded <today> at rev <new>)
       <old header verbatim>
  3. appends the §8 index note:  ", H<n+1> (rev <old>, the R<round> header —
     offloaded at rev <new>)" before the closing "**." of the index line
  4. if --new-header FILE is given: replaces handoff line 1 with the file's
     first paragraph (its own rev must be <new>)
  5. enforces the 16 KB gate (default 16384) at the end

--dry-run shows what would happen without writing. --today overrides the date
(default: the header's own date for H-titles; offload date = today).

Exit 0 = done (or dry-run clean), 1 = anchor/parse/gate problem.
"""
import argparse, datetime, os, re, sys

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
GATE = 16384
HDR_RE = re.compile(
    r'# main branch — handoff \(rev (\d+), (\d{4}-\d{2}-\d{2}) — \*\*R(\d+):? ([^*]*?)\*\*')


def read(p):
    with open(p, encoding='utf8') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf8', newline='') as f:
        f.write(t)


def main():
    ap = argparse.ArgumentParser(description='handoff rev-header offload')
    ap.add_argument('--repo', default=DEFAULT_REPO)
    ap.add_argument('--new-header', help='file whose content becomes handoff line 1')
    ap.add_argument('--today', help='offload date YYYY-MM-DD (default: actual today)')
    ap.add_argument('--gate', type=int, default=GATE)
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()

    hp = os.path.join(a.repo, 'handoff.md')
    hxp = os.path.join(a.repo, 'HISTORY.md')
    handoff = read(hp)
    history = read(hxp)

    m = HDR_RE.match(handoff)
    if not m:
        print('FAIL: handoff line 1 does not match the rev-header shape')
        return 1
    old_rev, hdr_date, rnd, slug = int(m.group(1)), m.group(2), m.group(3), m.group(4).strip()
    today = a.today or datetime.date.today().isoformat()

    h_nums = [int(x) for x in re.findall(r'^## H(\d+)\.', history, re.M)]
    new_h = (max(h_nums) + 1) if h_nums else 1

    new_rev = old_rev + 1
    if a.new_header:
        nh = read(a.new_header).split('\n')[0].strip()
        mn = re.match(r'# main branch — handoff \(rev (\d+)', nh)
        if not mn:
            print('FAIL: --new-header line 1 does not match the rev-header shape')
            return 1
        new_rev = int(mn.group(1))
        if new_rev <= old_rev:
            print(f'FAIL: new rev {new_rev} must be > old rev {old_rev}')
            return 1

    h_heading = (f'## H{new_h}. R{rnd} — {slug} ({hdr_date}) — verbatim handoff '
                 f'line at the rev-{old_rev} state (offloaded {today} at rev {new_rev})')
    h_record = h_heading + '\n\n' + handoff.split('\n')[0] + '\n'

    print(f'current : rev {old_rev}, R{rnd}, {len(handoff.encode("utf8"))} bytes')
    print(f'offload : H{new_h} -> HISTORY.md  ("R{rnd} — {slug}")')
    if a.new_header:
        print(f'header  : replace with {a.new_header} (rev {new_rev})')
    print(f'§8 index: append "H{new_h} (rev {old_rev}, the R{rnd} header — '
          f'offloaded at rev {new_rev})"')

    if re.search(r'^## H%d\.' % new_h, history, re.M):
        print(f'FAIL: H{new_h} already exists in HISTORY.md (offload already done?)')
        return 1

    # preview the size with the new header applied
    est = len(h_record.encode('utf8'))
    print(f'HISTORY gains ~{est} bytes; handoff '
          + (f'header swap {"shrinks" if a.new_header else "not requested"} — '
             f'gate re-check runs after the edit'))

    if a.dry_run:
        print('DRY-RUN: nothing written')
        return 0

    if not history.endswith('\n'):
        history += '\n'
    history += '\n' + h_record
    write(hxp, history)

    # §8 index: insert before the trailing "**." of the line containing 'H65 ('
    idx_anchor = re.search(r'(H\d+ \(rev \d+[^\n]*?)\*\*\.', history)  # sanity only
    h8 = re.search(r'\n- \*\*Docs policy.*', handoff, re.S)
    handoff_txt = handoff
    m8 = re.search(r', (H\d+ \(rev \d+, [^\n]*?)\*\*\.', handoff)
    if m8:
        note = (f', H{new_h} (rev {old_rev}, the R{rnd} header — offloaded at rev {new_rev})')
        handoff_txt = handoff[:m8.end() - len('**.')] + note + handoff[m8.end() - len('**.'):]
    else:
        print('WARN: §8 H-index tail not found — index not updated by script')

    if a.new_header:
        rest = handoff_txt.split('\n', 1)[1]
        handoff_txt = nh + '\n' + rest
        print(f'handoff: line 1 replaced (rev {new_rev})')

    write(hp, handoff_txt)
    size = len(handoff_txt.encode('utf8'))
    print(f'handoff size: {size} bytes (gate {a.gate})')
    if size > a.gate:
        print(f'FAIL: over the gate by {size - a.gate} bytes — trim the new '
              f'header/body (details live in the H-record now)')
        return 1
    print('OK')
    return 0


if __name__ == '__main__':
    sys.exit(main())
