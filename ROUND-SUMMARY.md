# Round summary — rev 152 (2026-11-09): the upstream-merge line retired — tree cleaned to D2VP-only

Owner: **"It is unlikely we are going to be merging again. Let's remove any extra files and
documentation not relevant to D2VP and not required to build."** Docs-only round: the
self-ID stays **`Vanilla+0.0.8-R138`** and the gates stay **525/525** (same rows — only
three merge-era comment strings inside the gate script were reworded).

## Removed (git history keeps every path)

- **Merge protocol + sync tooling**: `docs/merge-upstream.md` (the eleven-sync merge
  procedure), the whole `scripts/` dir (`merge_site_syntax_sweep.sh`,
  `fork_surface_syntax_sweep.sh`, `refile_aliasing_audit.py`, `include_path_audit.py`,
  `d2boot_visibility_audit.py`, `rev47_vp_placement_audit.py`, `rev48_wrong_branch_audit.py`,
  `rev49_trackc_provenance.sh`), `tools/hand-follow.py` (the regen hand-delta carrier).
  The owner's MSYS2 build remains the compile gate of record;
  `tools/syntax-check.sh` (+ `tools/vp_host_api.h`/`vp_host_shim.h`) keeps the nine-TU
  fork-surface host instrument.
- **The hand-port shelf**: `plans/ELEVENTH-SYNC-SALVAGE.md` — dies with the merge line;
  the `92f67c4f` / `a633e56a` consult items are DROPPED, not owed.
- **The upstream Redivivus agent plugin**: `agent/` (MCP server config + skill) and
  `.claude-plugin/` (marketplace manifest).
- **Recomp-era artifacts**: `asm_matching/` (the msvc8 byte-compare oracle, 1.2 MB),
  `progress/` + `.github/workflows/progress.yml` (the progress-badge CI),
  `.recomp.json`.

## Kept (build-required or D2VP-relevant)

`CMakeLists.txt`, `project.json`, `cmake/`, `vanilla-plus/cmake/` (the build's source
filters), `3rdparty/`, `Redivivus/`, `Source/`, `.github/workflows/build.yml` +
`tools/check-build.sh` (CI build), the SMOKE template chain
(`SMOKE-CHECKLIST.md` + `tools/gen-smoke-issue-template.py` + the issue template), the
D2VP docs lanes (README / CHANGELOG / handoff / HISTORY / FEATURES / VANILLA-PLUS /
CONTRIBUTING), `docs/building.md` + `docs/resolution-widelayout-audit.md` +
`docs/original-compilers.md` (trimmed), `tools/{check-vanilla-plus.sh, syntax-check.sh,
build-local-win11.*, vp_host_*.h}`, and the feature/issue plans
(`plans/{PLAN,1344x700-PLAN,SHARED-STASH-PLAN,SHIFT-COMPARE-PLAN,ISSUE31-*,ISSUE33-*,README}.md`).

## Surviving-reference edits

- `vanilla-plus/cmake/sources.cmake` — the `asm_matching/` exclusion line dropped
  (dated note left in the comment).
- `tools/check-vanilla-plus.sh` — the header advice, the SCmd tripwire comment and the
  failure epilogue no longer point at the removed doc/scripts (row count unchanged).
- `tools/syntax-check.sh` — the asm_matching include-exclusion dropped (dated note);
  one comment no longer names the removed merge-site sweep.
- **Fork-TU self-check pointers** (comment/string-literal text only, zero behavior
  change): `VanillaPlusCore.{h,cpp}` comments, the two `static_assert` messages and the
  six `D2BOOT_Fail` strings, plus the `VanillaPlusCombat.h` / `VanillaPlusStash.h`
  header comments and `Automap.cpp`'s VP-block comment now point at **handoff.md §8**
  instead of the removed merge playbook; the "audit the VP markers" wording replaces
  "audit against docs/merge-upstream.md".
- `VANILLA-PLUS.md` — principle 4 (keep current with upstream) → **RETIRED 2026-11-09**
  with the ten-sync history (the old text was also stale: "eight syncs → 1948").
- `Redivivus/README.md` — the libcmt libmap evidence note dated (file in git history).
- `.github/workflows/build.yml` — `paths-ignore` tidied (`.recomp.json`,
  `progress/**`, the progress workflow are gone); the manual-audit comment updated.
- `docs/original-compilers.md` — the compiler/flag tables stay (preservation
  knowledge); the oracle how-to replaced with a dated removal note (git history).
- `CONTRIBUTING.md` — the hand-written table drops `asm_matching/` + `agent/`; the
  upstream paragraph → retired; the house-rule intro reworded and its stale gate count
  corrected 520 → 525.
- `README.md` — the docs index drops `docs/merge-upstream.md`; the upstream paragraph
  → retired.
- **handoff.md → rev 152**: the retirement record (§1, §2 the sync row → RETIRED, §8
  H-index); outgoing rev-151 paragraph offloaded verbatim to HISTORY **H57**.
- **HISTORY.md**: **H57** = the rev-151 paragraph verbatim.

Historical records keep their wording on purpose: CHANGELOG R129/R130 entries and the
HISTORY archive describe what happened when the merge line was alive.

## Suggested commit split

1. `chore: retire the upstream merge line - remove sync tooling, salvage shelf, agent plugin, recomp-era artifacts`
   — all deletions + sources.cmake + build.yml + syntax-check.sh
2. `docs: merge line retired - rev 152; rev-151 offloaded to H57` — handoff.md,
   HISTORY.md, ROUND-SUMMARY.md
3. `docs: CONTRIBUTING/README/original-compilers to the retired-sync state` — the three
   docs

## Verification

- `tools/check-vanilla-plus.sh`: **525/525** (handoff within the 16 KB budget).
- Repo-wide sweep: no dangling references to the removed paths outside HISTORY.md /
  CHANGELOG historical entries, ROUND-SUMMARY (this file) and the dated removal notes.
- Source edits verified mechanically: the five touched fork TUs are comment /
  string-literal text only (quote-balance scan clean), zero behavior change; the CMake
  edit is a one-line list-exclusion removal.
- **Pre-existing, not this round**: `tools/syntax-check.sh`'s committed default TU list
  is stale at HEAD (pre-R130 paths, 4 TUs, and the shadow-`windows.h` include walk
  cannot resolve `Fog/Debug.h` in this sandbox) — identical failure on pristine
  `7b48ce0d`. The owner's MSYS2 build remains the compile gate of record; worth a
  repath pass in a future round if the host instrument is still wanted.

## Still owed (unchanged from rev 151)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE §N item 50, §O item 52 ride-alongs on any future build; §P item 54's 1366x768
  `-w` select doubles as the R116 eyeball.
- §7 triages: #25 (track R, shrine XP buff), #18 (track Q, attribute-point crash).
