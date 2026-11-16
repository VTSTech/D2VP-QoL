# D2VP-QoL — Audit Quick-Win Fixes (uncommitted, on top of 0951f99 / R158)

All changes are uncommitted working-tree edits. `python3 tools/docs_gate.py` exits **0**
(was failing on main for two pre-existing reasons — see below). Round/self-ID is still
R158; bump to R159 per your usual flow if these land.

## Code fixes

1. **SEC-01 — control channel binds loopback** (`Redivivus/d2_control.cpp`, `OpenListener`)
   `sin_addr` is now `htonl(0x7F000001)` (127.0.0.1) instead of INADDR_ANY. The
   unauthenticated channel (hex / cpkt / shot / guard) is no longer reachable from the
   network; a local agent connects exactly as before.

2. **SEC-02 — remote-input guards** (`Redivivus/d2_control.cpp`)
   - `mon`: `sscanf` return checked (error reply on malformed line) and start/count
     clamped to >= 0 — a negative start no longer indexes `pTxtMonStats` OOB.
   - `watch`: the reply's target dereference is now behind the same `IsBadReadPtr`
     contract the `hex` command uses — `watch 1` returns value 0 instead of faulting.

3. **MAINT-05 — dead code removed**
   - `if (0) wsprintfA(...)` statement deleted (`d2_control.cpp`).
   - Unused `#define DEFAULT_SECURITY_COOKIE` deleted (`VanillaPlusOptions.cpp`).

4. **First-poll guard** — `VANILLAPLUS_GlobeTextPersistent` now has the
   `!lastCheck ||` first-poll guard every other 2 s gate has (pattern uniformity).

## Comment / doc fixes (behavior unchanged)

5. **ROB-07** — `VanillaPlusStash.h` no longer claims a dead/trading refusal that isn't
   implemented; states the real behavior instead.
6. **MAINT-04** — ini template clock comment now matches the shipped one-line 12-hour
   R148 readout; `VanillaPlusCore.h` options-page geometry comment updated to the R155
   two-columns-of-eight / R158 sixteen-row reality; `Redivivus/README.md` `__ftol2`
   section updated (it is implemented in `d2_crt_impl.cpp` now; original rationale kept
   for the record).
7. **MAINT-01** — `CONTRIBUTING.md` no longer cites the retired
   `tools/check-vanilla-plus.sh` as the enforcement suite; documents what actually
   checks what today (docs_gate.py, check-build.sh, owner Win11 build).

## Marker hygiene (ROB-09)

8. Added the three missing `// VP-END` closers: `D2Client.cpp` `[issue #26 / R92]`,
   `ui.cpp` `[issue-18 fmt-args]`, `Wall2.h` `[R118]`. Repo-wide marker balance is now
   147 BEGIN / 147 END; docs_gate pairing PASSES on all three files. (Path.cpp's nested
   pairs were left alone — they are balanced.)

## Gate made green (two pre-existing failures on main)

9. `docs_gate.py` FEATURES check: historical bullets' `**Thirteen/Fifteen features
   ship.**` reworded to past tense outside the bold marker ("...features shipped as of
   this round") so the row-count parity check (16) passes; history preserved.
10. Smoke sync contract: ran the repo's own `tools/smoke_sync.py copy --section W` to
    sync SMOKE §W (R158 Extended Stats) into `.github/ISSUE_TEMPLATE/smoke-test.md`.

## Not touched (deliberately)

- ROB-01/ROB-02 (the two High findings) — real logic changes in the stash write path;
  want your review + a field round, not a drive-by fix.
- Remaining `sscanf` sites in d2_control (`ai`, `setclip`, `walldir`, `objname`) —
  same class as the `mon` fix, left for a dedicated pass.
- MAINT-06 self-ID composition, ARCH-01 gate-skeleton standardization, TEST-01/02 —
  short-term items, need design calls.
