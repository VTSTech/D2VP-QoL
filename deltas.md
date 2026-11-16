# D2VP-QoL — Closed Audit Findings (Moved from audit.md)

This file tracks audit findings that have been resolved and moved from audit.md. These were implemented in rounds R159, R160, and R161.

## Fixed Findings

| ID | Round | Severity | Category | Title | What was Implemented |
|----|-------|----------|----------|-------|---------------------|
| SEC-01 | R159 | Medium | Security | DEV control channel binds unauthenticated listener on 0.0.0.0 | DEV control channel binds loopback only |
| SEC-02 | R159 | Medium | Security | Control-channel commands parse remote input unsafely (deref, negative index) | `mon`/`watch` remote-input guards |
| SEC-02 (residual) | R161 | Medium | Security | (continued from R159) | `ai`/`walldir`/`objname`/`setclip` parse guards + clamps; `sym` help stub retired |
| SEC-03 | R161 | Low | Security | `d2_boot` path assembly with `lstrcpyA`/`lstrcatA` can overrun by ~11 bytes | boot-log path assembly bounded (exe-dir + %TEMP% fallbacks reserve the suffix room) |
| ROB-01 | R160 | High | Robustness | Transient stash read failure pins degraded state; next save overwrites good file | transient `.sss` read failure defers (toggle + save refused until a read succeeds) |
| ROB-02 | R160 | High | Robustness | Partial page materialization re-serializes incomplete set, dropping records | partial materialization keeps the stored block authoritative |
| ROB-04 | R161 | Medium | Robustness | Personal stash is memory-only while a shared page is active | the memory-only personal-stash exposure is documented in README + FEATURES |
| ROB-05 | R161 | Medium | Robustness | `wvsprintfA` 1024-char output cap overruns undersized boot/crash log buffers | diagnostics pre-truncation: BootLogWrite caps inputs at 580, MarkFmt/TraceFmt bodies sized to wvsprintfA's 1024 cap, D2Trace line buffer 1100; stale d2_crash comment fixed |
| ROB-06 | R161 | Medium | Robustness | Failed ini writes are silent; Windows profile cache defeats the hot-read contract | ini write result checked and returned; Options row plays a distinct sound on failure; Windows profile-cache caveat documented (gate comment + ini template) |
| ROB-07 | R159 | Medium | Robustness | Stash header claims dead/trading refusal that is not implemented | header no longer claims an unimplemented dead/trading refusal |
| ROB-08 | R161 | Medium | Robustness | `VANILLAPLUS_SelfCheck` calls `exit(-1)` mid-game on first ini poll | `VANILLAPLUS_SelfCheck` runs at Game/Main.cpp boot, before any game state; the EnsureIni site stays as the once-guarded belt |
| ROB-09 | R159 | Low | Robustness | Three unbalanced VP-BEGIN markers; two nested marker pairs | three unbalanced VP markers closed |
| ROB-10 | R161 | Low | Robustness | Bounded-but-fragile buffer idioms (ilvl suffix, label buffers, walldir slot) | `ITEMILVL_AppendNameSuffix` takes an explicit capacity; ShowItems label cap 0x100→0xFE (terminator slack); shift-compare channel size pinned by static_assert; the two suspected SFile registry-migration reads wrapped in VP note markers (byte-identical, upstream-visible) |
| MAINT-01 | R159 | Medium | Maintainability | CONTRIBUTING cites retired gate script `tools/check-vanilla-plus.sh` | CONTRIBUTING names what actually enforces what |
| MAINT-02 | R161 | Medium | Maintainability | Adding a feature requires 4 coordinated edits; in-code comment understates it | the Options-page "+1 entry, nothing else" comment now lists all four coordinated edits |
| MAINT-04 | R159 | Low | Maintainability | Stale doc comments (clock format, options geometry, README `__ftol2`) | stale doc comments fixed (clock, options geometry, `__ftol2`) |
| MAINT-05 (part) | R159 | Low | Maintainability | Dead code inventory: parked diagnostics, unused macro, `if (0)`, `sym` command | unused macro + `if (0)` statement deleted |
| MAINT-06 | R161 | Low | Maintainability | Hardcoded "0.0.8" literal in boot self-ID drifts from build tag | `D2VP_VERSION_TAG` composes once in d2_boot.h; boot self-ID derives from it |
| TEST-01 | R161 | Medium | Testing | Gate suite retired; marker hygiene now only checked on git-changed files | `tools/check-vanilla-plus.sh` revived as the reduced suite (docs_gate + whole-repo marker census + LF check) |
| FEAT-02 (minimal) | R161 | Medium | New Feature | User-visible feedback for failed settings writes | folded into ROB-06 (failure sound + d2boot mark) |
| FEAT-03 (partial) | R161 | Low | New Feature | Compile-time tripwires tying resolution-ladder constants together | `VPRES_MAX_HEIGHT` + static_assert pin the row-table sizing (the #31 class) |
| FEAT-04 | R161 | Low | New Feature | Hot-reload parity (or documentation) for restart-only settings | ini template gains the `[ui] label_font` block marked REQUIRES RESTART |
| ARCH-01 (part) | R159 | Medium | Architecture | 2-second ini-gate pattern duplicated across the module boundary | the GlobeTextPersistent first-poll guard drift fixed; pattern uniform |

## Summary

- **Total Closed Findings**: 24 findings
- **High Severity**: 2 (ROB-01, ROB-02)
- **Medium Severity**: 14 
- **Low Severity**: 8
- **Distribution by Round**: R159 (9), R160 (2), R161 (13)

All findings listed above have been implemented and the corresponding sections have been removed from audit.md to keep it focused on outstanding issues.