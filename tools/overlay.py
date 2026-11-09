#!/usr/bin/env python3
"""overlay.py — pack / verify the per-round overlay zip (round-agnostic).

The workflow: sandbox edits the tree -> overlay zip of the CHANGED files ->
the owner applies it over their tree and builds on Win11 MSYS2 (the compile
gate of record). Replaces the per-round make_rXXX/verify_rXXX one-offs.

  pack   --round N [--out DIR] [--all]
      zip every git-changed tracked file (or also untracked with --all) into
      <out>/d2vp-r<N>-overlay.zip. Refuses an empty change set; never
      overwrites silently (delete the old zip first).

  verify --round N [--marker SUBSTR]... [--absent SUBSTR]
      entry set == git-changed set; every entry byte-identical to the working
      tree; boot carries exactly one D2VP_ROUND define equal to N;
      every --marker substring present in the zipped bytes; every --absent
      substring absent from the zipped boot file.

Exit 0 = ok, 1 = any failure.
"""
import argparse, hashlib, os, re, subprocess, sys, zipfile

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
# output dir defaults to <repo-parent>/download; --out overrides
DEFAULT_OUT = os.path.join(os.path.dirname(DEFAULT_REPO), 'download')


def changed(repo, include_untracked=False):
    cmd = ['git', 'status', '--porcelain']
    out = subprocess.run(cmd, cwd=repo, capture_output=True, text=True).stdout
    paths = []
    for line in out.splitlines():
        if not line.strip():
            continue
        st, p = line[:2], line[3:].strip()
        if include_untracked or not st.strip() == '??':
            if os.path.isfile(os.path.join(repo, p)):
                paths.append(p)
    return sorted(set(paths))


def sha(b):
    return hashlib.sha256(b).hexdigest()


def cmd_pack(a):
    paths = changed(a.repo, a.all)
    if not paths:
        print('FAIL: no changed files — nothing to pack')
        return 1
    out = os.path.join(a.out, f'd2vp-r{a.round}-overlay.zip')
    if os.path.exists(out):
        print(f'FAIL: {out} exists — delete it first (never overwrite silently)')
        return 1
    os.makedirs(a.out, exist_ok=True)
    with zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for p in paths:
            z.write(os.path.join(a.repo, p), p)
    print(f'packed {len(paths)} files -> {out} ({os.path.getsize(out)} bytes)')
    for p in paths:
        print('  + ' + p)
    return 0


def cmd_verify(a):
    zp = os.path.join(a.out, f'd2vp-r{a.round}-overlay.zip')
    if not os.path.exists(zp):
        print(f'FAIL: {zp} not found')
        return 1
    z = zipfile.ZipFile(zp)
    names = sorted(z.namelist())
    expect = changed(a.repo, a.all)
    fail = []
    if names != sorted(expect):
        fail.append(f'entry set != changed set (zip {len(names)} vs tree {len(expect)}; '
                    f'only-zip: {sorted(set(names) - set(expect))}; '
                    f'only-tree: {sorted(set(expect) - set(names))})')
    for rel in names:
        p = os.path.join(a.repo, rel)
        if not os.path.exists(p):
            fail.append(rel + ': gone from tree')
            continue
        if sha(z.read(rel)) != sha(open(p, 'rb').read()):
            fail.append(rel + ': bytes differ from tree')
    boot_rel = 'Redivivus/d2_boot.h'
    if boot_rel in names:
        boot = z.read(boot_rel).decode('utf8', 'replace')
        defines = re.findall(r'#define D2VP_ROUND "(\d+)"', boot)
        if defines != [str(a.round)]:
            fail.append(f'boot D2VP_ROUND {defines} != ["{a.round}"]')
        for sub in a.absent:
            if sub in boot:
                fail.append(f'boot: forbidden substring present: {sub!r}')
    else:
        fail.append('boot file missing from zip')
    blob = {n: z.read(n) for n in names}
    for sub in a.marker:
        if not any(sub.encode('utf8') in b for b in blob.values()):
            fail.append(f'marker not found in any zipped file: {sub!r}')
    if fail:
        print(f'VERIFY R{a.round}: {len(fail)} problem(s)')
        for f in fail:
            print('  FAIL ' + f)
        return 1
    print(f'VERIFY R{a.round}: OK — {len(names)} entries, byte-identical to the tree, '
          f'{len(a.marker)} marker(s) present')
    return 0


def main():
    ap = argparse.ArgumentParser(description='overlay zip pack/verify')
    sub = ap.add_subparsers(dest='cmd', required=True)

    pp = sub.add_parser('pack')
    pp.add_argument('--round', type=int, required=True)
    pp.add_argument('--repo', default=DEFAULT_REPO)
    pp.add_argument('--out', default=DEFAULT_OUT)
    pp.add_argument('--all', action='store_true', help='include untracked files')

    pv = sub.add_parser('verify')
    pv.add_argument('--round', type=int, required=True)
    pv.add_argument('--repo', default=DEFAULT_REPO)
    pv.add_argument('--out', default=DEFAULT_OUT)
    pv.add_argument('--marker', action='append', default=[],
                    help='substring that must exist in the zipped bytes (repeatable)')
    pv.add_argument('--absent', action='append', default=[],
                    help='substring that must NOT appear in the zipped boot (repeatable)')
    pv.add_argument('--all', action='store_true')

    a = ap.parse_args()
    if not os.path.isdir(a.repo):
        print(f'FAIL: repo not found: {a.repo}')
        return 1
    return cmd_pack(a) if a.cmd == 'pack' else cmd_verify(a)


if __name__ == '__main__':
    sys.exit(main())
