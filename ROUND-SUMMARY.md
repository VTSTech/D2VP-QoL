# Round summary — R161 (rev 177, 2026-11-16): audit closure — every remaining safe fix

## The ask

No owner ask — this round closes out the audit (`audit.md`, 22 findings): the
ledger at `AUDIT-FIXES.md` now resolves every finding to fixed (R159/R160/R161)
or deferred-with-reason. R161 lands everything that could be fixed without a
design call; no healthy-path behavior changes.

## Diagnostics hardening (ROB-05 + SEC-03)

- `BootLogWrite` pre-truncates every input at 580 chars (a crash-report line
  routed through `D2BOOT_Mark` can be 1023 chars — it overran the 640-byte
  stack buffer exactly on the dying-process path, where the report silently
  corrupted).
- `D2BOOT_MarkFmt` / `D2TraceFmt` bodies sized to `wvsprintfA`'s own 1024-char
  output cap; `D2Trace`'s line buffer to 1100 (cap + prefix).
- Boot-log path assembly bounded in both fallbacks (exe dir + %TEMP%): the
  suffix gets 11 reserved bytes, closing SEC-03's ~11-byte stack overrun on
  near-260-char install paths.
- The stale `d2_crash.cpp` comment that described truncation as already
  implemented now matches the enforced reality.

## Behavior-visible fixes

- **ROB-08**: `VANILLAPLUS_SelfCheck` runs at `Game/Main.cpp` boot, before any
  game state — a tripped tripwire is a clean `d2boot.log` diagnosis at startup
  instead of a mid-game `exit(-1)` from the first ini poll. The
  `ALTTOGGLE_EnsureIni` call site stays as the once-guarded belt.
- **ROB-06 + FEAT-02 (minimal)**: `D2VP_SetFeatureToggle` returns the WRITE
  result; a failed write (read-only game directory) plays the button-click
  sound instead of the select chime and logs one `[d2boot]` line. The Windows
  profile-cache caveat (external ini edits may not be seen after the first
  in-game toggle) is documented in the gate comment and the ini template.
- **SEC-02 residual**: `ai` / `walldir` / `objname` / `setclip` parse remote
  input with guards + clamps (the R159 `mon` pattern); the never-implemented
  `sym` help line retired.
- **ROB-10**: `ITEMILVL_AppendNameSuffix` takes an explicit capacity (BYTE
  units, the sites' `GetItemName` convention); the ShowItems ground-label cap
  drops 0x100 → 0xFE (at max-length names the terminator wrote one slot past
  the 128-wchar array); the shift-compare tooltip channel size is pinned by a
  static_assert; the two suspected upstream registry-migration reads in
  `SFile.cpp` are documented under VP note markers, byte-identical.

## Structure + docs

- **MAINT-06**: `D2VP_VERSION_TAG` composes once in `d2_boot.h`; the boot
  self-ID (`"Vanilla+" D2VP_VERSION_TAG`) and the title/automap tag derive from
  one header — a release bump touches one place.
- **FEAT-03 (partial)**: `VPRES_MAX_HEIGHT` + a static_assert pin
  `gaYBufferRowOffsets` at 1080 + 0x20 — the issue-#31 class is a compile
  error now.
- **TEST-01**: `tools/check-vanilla-plus.sh` revived as the reduced suite:
  docs_gate + whole-repo VP-marker census (149/149) + LF normalization.
- **MAINT-02 / ARCH-02 / ROB-04 / FEAT-04**: comment and doc fixes — the
  four-edit feature checklist, the mirror-desync note at `gnVpSharedPage`, the
  memory-only personal-stash exposure in README/FEATURES, the `[ui]
  label_font` REQUIRES-RESTART block in the ini template.

## Deferred (see AUDIT-FIXES.md for the full reasons)

ROB-03 (ownership epoch — needs a teardown hook), MAINT-05 parked diagnostics
(deletion interleaves with live code; no cross-compiler in the audit sandbox),
MAINT-03/07 (mirror/naming churn), ARCH-01 doc + ARCH-02 echo, PERF-01/02
(documented no-action), TEST-02/03 (harness infrastructure), FEAT-01 (session
journal — a feature with its own round).

## Verification

Formal ask: SMOKE **§Y 100–101** (boot self-ID + healthy Options-toggle pass;
the standing belts cover the rest — every fix here is a failure-path or
comment change). Gate: `tools/docs_gate.py` green at round R161;
`tools/check-vanilla-plus.sh` sections 2–3 green (marker census + LF).
