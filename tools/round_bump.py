#!/usr/bin/env python3
"""round_bump.py — roll the round number forward, mechanically.

The prose (handoff header, CHANGELOG entry, ROUND-SUMMARY, section text) is
written by the agent; this script handles the mechanical bump + the live-doc
census so nothing is forgotten:

  --set N            rewrite D2VP_ROUND in Redivivus/d2_boot.h to N (keeping
                     any trailing // comment); refuses if already N
  --report N         census of the PREVIOUS round's live references (self-IDs,
                     D2VP-R tags, CHANGELOG top bullet, ROUND-SUMMARY title,
                     SMOKE sections) — what still needs the agent's pen

Run order in a round: docs first (agent), then `round_bump --set N`, then
`docs_gate --round N` until green.

Exit 0 = ok, 1 = refusal or parse failure.
"""
import argparse, os, re, sys

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
BOOT_REL = 'Redivivus/d2_boot.h'
LIVE_DOCS = ['CHANGELOG.md', 'README.md', 'FEATURES.md', 'SMOKE-CHECKLIST.md',
             'ROUND-SUMMARY.md', 'handoff.md',
             '.github/ISSUE_TEMPLATE/smoke-test.md']


def read(p):
    with open(p, encoding='utf8') as f:
        return f.read()


def write(p, t):
    with open(p, 'w', encoding='utf8', newline='') as f:
        f.write(t)


def current(repo):
    m = re.search(r'#define D2VP_ROUND "(\d+)"', read(os.path.join(repo, BOOT_REL)))
    return int(m.group(1)) if m else None


def cmd_set(a, n):
    p = os.path.join(a.repo, BOOT_REL)
    boot = read(p)
    cur = current(a.repo)
    if cur is None:
        print('FAIL: D2VP_ROUND not parseable')
        return 1
    if cur == n:
        print(f'boot already at R{n} — nothing to do')
        return 0
    new_boot, cnt = re.subn(r'(#define D2VP_ROUND ")\d+(")', r'\g<1>%d\g<2>' % n, boot)
    if cnt != 1:
        print(f'FAIL: expected exactly one define, found {cnt}')
        return 1
    if a.note:
        # replace or append the trailing // comment on the define line
        def _note(m):
            return m.group(1) + ' ' + a.note
        new_boot = re.sub(r'(#define D2VP_ROUND "\d+").*$', _note, new_boot, count=1)
    write(p, new_boot)
    print(f'boot: D2VP_ROUND "{cur}" -> "{n}"' + (f' (+ note)' if a.note else ''))
    return 0


def cmd_report(a, n):
    repo = a.repo
    print(f'== live-doc census: references to R{n - 1} (previous round) ==')
    hits = 0
    for rel in LIVE_DOCS:
        p = os.path.join(repo, rel)
        if not os.path.exists(p):
            print(f'  MISSING {rel}')
            continue
        txt = read(p)
        for i, line in enumerate(txt.split('\n'), 1):
            if re.search(r'R%d\b' % (n - 1), line):
                hits += 1
                scope = 'LIVE' if rel == 'handoff.md' and i == 1 else 'note'
                prev = line.strip()
                print(f'  {rel}:{i} [{scope}] {prev[:110]}{"…" if len(prev) > 110 else ""}')
    # specific live positions that MUST move
    boot = read(os.path.join(repo, BOOT_REL))
    cur = current(repo)
    print(f'\nboot D2VP_ROUND: "{cur}"' + ('  (== target, OK)' if cur == n else f'  (target {n} — run --set {n})'))
    handoff = read(os.path.join(repo, 'handoff.md'))
    m = re.match(r'# main branch — handoff \(rev (\d+), \d{4}-\d{2}-\d{2} — \*\*R(\d+)', handoff)
    if m:
        ok = int(m.group(2)) == n
        print(f'handoff header: rev {m.group(1)}, R{m.group(2)}' + ('  (OK)' if ok else f'  (target {n} — needs the agent\'s new header)'))
    s1 = handoff[handoff.find('## 1.'):handoff.find('## 2.')]
    selfids = set(re.findall(r'Vanilla\+0\.0\.8-R(\d+)', s1))
    print(f'handoff §1 self-IDs: {sorted(selfids)}' + ('  (OK)' if selfids == {str(n)} else f'  (target {{{n}}})'))
    changelog = read(os.path.join(repo, 'CHANGELOG.md'))
    top = re.search(r'^- \*\*R(\d+)', changelog, re.M)
    print(f'CHANGELOG top bullet: R{top.group(1) if top else "?"}')
    summary = read(os.path.join(repo, 'ROUND-SUMMARY.md'))
    tm = re.match(r'# Round summary — R(\d+)', summary)
    print(f'ROUND-SUMMARY title: R{tm.group(1) if tm else "?"}')
    print(f'\n{hits} R{n - 1} reference(s) across live docs — historical mentions '
          f'are fine; self-IDs, headers and top bullets must move.')
    return 0


def main():
    ap = argparse.ArgumentParser(description='mechanical round bump + census')
    ap.add_argument('--set', type=int, metavar='N', help='rewrite D2VP_ROUND to N')
    ap.add_argument('--note', help='trailing // comment for the define (with --set)')
    ap.add_argument('--report', type=int, metavar='N',
                    help='census of R<N-1> references (N = the NEW round)')
    ap.add_argument('--repo', default=DEFAULT_REPO)
    a = ap.parse_args()
    if not os.path.isdir(a.repo):
        print(f'FAIL: repo not found: {a.repo}')
        return 1
    if not (a.set or a.report):
        ap.print_help()
        return 0
    if a.report:
        return cmd_report(a, a.report)
    return cmd_set(a, a.set)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except BrokenPipeError:
        sys.exit(0)
