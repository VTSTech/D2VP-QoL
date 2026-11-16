# D2VP-QoL — Audit closure ledger (R159 + R160 + R161)

Source: `audit.md` (the full code audit, 22 findings). This ledger tracks every
finding to its resolution. R159 = quick wins, R160 = the two Highs,
R161 = everything else that could be fixed without a design call.

## Fixed

| Finding | Round | What landed |
|---|---|---|
| SEC-01 | R159 | DEV control channel binds loopback only |
| SEC-02 | R159 | `mon`/`watch` remote-input guards |
| SEC-02 (residual) | R161 | `ai`/`walldir`/`objname`/`setclip` parse guards + clamps; `sym` help stub retired |
| SEC-03 | R161 | boot-log path assembly bounded (exe-dir + %TEMP% fallbacks reserve the suffix room) |
| ROB-01 | R160 | transient `.sss` read failure defers (toggle + save refused until a read succeeds) |
| ROB-02 | R160 | partial materialization keeps the stored block authoritative |
| ROB-04 | R161 | the memory-only personal-stash exposure is documented in README + FEATURES |
| ROB-05 | R161 | diagnostics pre-truncation: BootLogWrite caps inputs at 580, MarkFmt/TraceFmt bodies sized to wvsprintfA's 1024 cap, D2Trace line buffer 1100; stale d2_crash comment fixed |
| ROB-06 | R161 | ini write result checked and returned; Options row plays a distinct sound on failure; Windows profile-cache caveat documented (gate comment + ini template) |
| ROB-07 | R159 | header no longer claims an unimplemented dead/trading refusal |
| ROB-08 | R161 | `VANILLAPLUS_SelfCheck` runs at Game/Main.cpp boot, before any game state; the EnsureIni site stays as the once-guarded belt |
| ROB-09 | R159 | three unbalanced VP markers closed |
| ROB-10 | R161 | `ITEMILVL_AppendNameSuffix` takes an explicit capacity; ShowItems label cap 0x100→0xFE (terminator slack); shift-compare channel size pinned by static_assert; the two suspected SFile registry-migration reads wrapped in VP note markers (byte-identical, upstream-visible) |
| MAINT-01 | R159 | CONTRIBUTING names what actually enforces what |
| MAINT-02 | R161 | the Options-page "+1 entry, nothing else" comment now lists all four coordinated edits |
| MAINT-04 | R159 | stale doc comments fixed (clock, options geometry, `__ftol2`) |
| MAINT-05 (part) | R159 | unused macro + `if (0)` statement deleted |
| MAINT-06 | R161 | `D2VP_VERSION_TAG` composes once in d2_boot.h; boot self-ID derives from it |
| TEST-01 | R161 | `tools/check-vanilla-plus.sh` revived as the reduced suite (docs_gate + whole-repo marker census + LF check) |
| FEAT-02 (minimal) | R161 | folded into ROB-06 (failure sound + d2boot mark) |
| FEAT-03 (partial) | R161 | `VPRES_MAX_HEIGHT` + static_assert pin the row-table sizing (the #31 class) |
| FEAT-04 | R161 | ini template gains the `[ui] label_font` block marked REQUIRES RESTART |
| ARCH-01 (part) | R159 | the GlobeTextPersistent first-poll guard drift fixed; pattern uniform |

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
