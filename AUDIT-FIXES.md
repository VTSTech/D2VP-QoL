# D2VP-QoL — Audit closure ledger (R159 + R160 + R161)

Closed audit findings have been moved to `deltas.md` for archival. This file now serves as a historical reference.

For current active findings, see `audit.md`. For completed findings, see `deltas.md`.

## Deferred (with reasons)

| Finding | Why deferred |
|---|---|
| ROB-03 (ownership epoch) | needs a per-game epoch field or a game-teardown hook; the recovered `SGAMEDATA` definition is not accessible in the headers, and adding a teardown hook is a behavior surface that deserves its own field round. The pointer-identity reset already handles the cross-game case; only same-address allocator reuse remains. |
| MAINT-05 (parked diagnostics, ~250 lines in d2_control.cpp) | the dead functions interleave with LIVE ones (`D2ProbeArmClipHwbp*` is called from the dispatch and the thread trampoline); deleting them unverified is exactly how a fork TU breaks. No cross-compiler in the audit sandbox — needs a compile-verified deletion pass. |
| MAINT-03 (mirror dedup + sync check) | the mirrors exist for real module-boundary reasons; the Options/Core extraction is a refactor with regression risk and zero user-visible gain. A docs_gate site-count check is a nice-to-have. |
| MAINT-07 (naming warts) | cosmetic renames across module boundaries (`VPSHADOW`, `VPSTASH_PAGES/PAGE_COUNT`, ScreenSize re-exports) — churn with zero behavior change. The `gbAltLatched` concern is already handled by `ALTTOGGLE_Reset`. |
| ARCH-01 (standardization doc + gate grep) | the drift fix landed (R159); the canonical skeleton doc + gate grep is convention work best done with the next new gate. |
| ARCH-02 (0x9C page echo) | needs server-side packet work for a non-SP scenario that does not exist yet; the mirror is documented at the declaration (R161). |
| PERF-01 | documented no-action (batch the row reads if the slate grows past ~24 features). |
| PERF-02 | documented no-action (one-way `timeBeginPeriod` is correct for a game process). |
| FEAT-01 (session journal) | a feature (save-on-flip journal), not a fix — needs its own plan + field round. ROB-04's exposure is documented meanwhile. |
| FEAT-02 (red-row flash) | the minimal sound+mark version shipped; the red-row draw is polish. |
| FEAT-03 (CUnit named constants) | the row-table assert shipped; naming the 400/800 panel-space literals is cosmetic. |
| TEST-02 (stash harness) | host-side harness with pool stubs — infrastructure that needs its own round; TEST-01's wrapper restored the cheap half of the coverage. |
| TEST-03 (mock link check + CI docs) | the original `scripts/mock_link_check.cpp` is not in the tree to commit; recreating it needs a compile environment. |
