# Improvement & Enhancement Audit

**D2VP-QoL (Vanilla+QoL) v0.0.8-R158**

**Repository:** https://github.com/VTSTech/D2VP-QoL
**Author:** VTSTech (fork of jaenster/redivivus) | **License:** see LICENSE | **Date:** 2026-11-16 (commit 0951f99)
**12 Findings** | **7 Categories** | SEC, ROB, MAINT, PERF, FEAT, ARCH, TEST

---

## Table of Contents

- [Executive Summary](#executive-summary)
- [Findings Summary](#findings-summary)
- [Detailed Findings](#detailed-findings)
  - [Security](#security)
  - [Robustness](#robustness)
  - [Maintainability](#maintainability)
  - [Performance](#performance)
  - [New Features](#new-features)
  - [Architecture](#architecture)
  - [Testing](#testing)
- [Priority Matrix](#priority-matrix)
- [Architecture Strengths](#architecture-strengths)

---

## Executive Summary

D2VP-QoL is a fork of jaenster/redivivus — the recovered Diablo II 1.14d C++ source (~771K lines of ghidra-mcp-generated code under `Source/`) — carrying a hand-written "Vanilla+" layer of crash fixes, a widescreen resolution ladder, and sixteen opt-in quality-of-life features gated behind a hot-polled `d2vp.ini`. This audit covered the hand-written surface in depth: the settings core (`Source/D2Client/UI/VanillaPlusCore.{h,cpp}`), the Options page, the shared stash (`Source/D2Game/PLAYER/VanillaPlusStash.cpp`), the portable-save hook (`Source/Fog/Source/SFile.cpp`), the Redivivus support code (boot, crash reporter, patcher, DEV control channel), the 147 VP-marked regions across 49 upstream files, and the build/gate wiring. The 771K generated lines were treated as read-only context per the repo's own house rules, not audited line-by-line.

The audit produced 31 findings: 2 High, 16 Medium, 13 Low. **24 findings have been resolved in rounds R159-R161.** Both High findings are data-integrity risks in the shared-stash subsystem: a transient read failure of `_d2vp_shared.sss` pins an all-empty in-memory state for the session, and the next save overwrites the good on-disk file (the one-generation `.bak` is the only recourse); and a partially-failed page materialization re-serializes the incomplete set over the full block, permanently dropping the records that failed to decode. The most consequential Medium cluster sits in the DEV-only control channel (`Redivivus/d2_control.cpp`), which binds an unauthenticated TCP listener on 0.0.0.0:7777 with commands for arbitrary memory reads, raw packet injection and arbitrary file writes — correctly compiled out of release builds, but exposed to the local network whenever a DEV build runs. The rest of the Medium set is a mix of silent-failure paths (ini write failures produce no user feedback, and the Windows profile cache can defeat the documented "external editor" hot-read contract), boot-path buffer-formatting limits (`wvsprintfA`'s 1024-char output cap against undersized stack buffers), and documentation drift (`CONTRIBUTING.md` still cites the retired 525-check gate script that no longer exists in `tools/`).

The overriding pattern across the codebase is unusually high discipline: every fork edit is tagged `[R###]`/`[#issue]` between `// VP-BEGIN`/`// VP-END` markers, every feature is opt-in with byte-vanilla defaults, every crash fix lands at a choke point with a written analysis, and the docs encode a deliberate two-lane policy (concise human docs, verbose agent memory). The gaps are correspondingly systematic too: correctness currently rests on the owner's field verification and static re-reads rather than any automated test, and the retired gate suite means marker/doc hygiene checks now only run on files that happen to change.

---

## Findings Summary

| ID | Severity | Category | Title |
|----|----------|----------|-------|
| ROB-03 | Medium | Robustness | Stash ownership pin uses pointer identity; address reuse resurrects stale pool pointers |
| MAINT-03 | Medium | Maintainability | Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair |
| TEST-02 | Medium | Testing | No automated tests for stash serialization / save invariants |
| FEAT-03 | Low | New Feature | Compile-time tripwires tying resolution-ladder constants together |
| FEAT-04 | Low | New Feature | Hot-reload parity (or documentation) for restart-only settings |
| ARCH-02 | Low | Architecture | Client shared-page mirror drifts from authoritative server page by design |
| TEST-03 | Low | Testing | Ad-hoc link checks (`mock_link_check.cpp`) not committed; no CI artifact test |

Severity levels: **High** — affects correctness, security, or data integrity; fix soon. Medium — impacts maintainability, reliability, or UX; address in planned work. Low — nice-to-have; address opportunistically.

---

## Detailed Findings

### Security


---

### Robustness

#### ROB-03: Stash ownership pin uses pointer identity; address reuse resurrects stale pool pointers

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:195-201` |

`VPSHARED_EnsureOwner` pins the BSS stash state to the owning `SGAMEDATA` by comparing `pOwnerGame == pGame`. If the allocator recycles the same `SGAMEDATA` address for a new game, the stale state — including `pPersonalBlock`/`apSharedBlock` pointers into the old, freed pool — is resurrected, and subsequent `D2_FREE_POOL_AT` calls would free old-pool pointers against the new game's pool (heap corruption).

**Recommendation:** store a generation/epoch counter (the game struct has one, or maintain a fork-side counter bumped on game teardown) alongside the pointer, and require both to match.

**Impact:** Prevents a cross-game heap-corruption class that would present as mysterious crashes minutes into a second game.

---

#### MAINT-03: Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Robustness |
---

### Maintainability

#### MAINT-03: Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:413-453` vs `VanillaPlusCore.cpp:1572-1612`; `Options.cpp:477-591` vs `Core.cpp:1617-1717`; `Source/D2Direct3D` vs `Source/D2Glide` camera-zoom sites (R155/R156) |

### Performance

### Performance

#### PERF-01: Options page polls 16 ini gates per frame while open

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Performance |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:787-798` |

While the Options page is open, its draw loop calls `D2VP_GetFeatureToggleDefault` 16 times per frame (`:798`). Each call is itself a 2-second cached poll, so steady-state cost is a tick compare — but each cache expiry re-parses `d2vp.ini`, and with 16 gates spread across frames, a pathologically large ini file produces periodic draw stutter on the page. Practically harmless at the current ~2 KB template size; noted because the pattern scales with feature count.

**Recommendation:** none needed now; if feature count grows past ~24, batch the row reads through one shared 2 s poll of the whole `[features]` section.

**Impact:** Keeps the Options page frame-stable as the feature slate grows.

---

#### PERF-02: `timeBeginPeriod(1)` is a one-way latch, never undone

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Performance |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:1150, 1182-1186` |

Once Unlock FPS is ever enabled (even toggled off mid-session), the process-wide — and system-wide on pre-Win10-2004 machines — timer resolution stays raised for the process lifetime; `timeEndPeriod` is never called. Deliberate and documented, and correct for a game process that exits cleanly, but it changes Sleep-based pacing for every subsystem in the process from the first flip onward.

**Recommendation:** leave as-is for gameplay; if it ever interferes with a subsystem (e.g. sound pacing anomalies), call `timeEndPeriod(1)` on gate-off and re-arm on gate-on.

**Impact:** Documented global side effect; no action required unless a pacing bug appears.

---

### Testing


---


---


---

## Priority Matrix

The project versions by code round (R###), currently R158. Timelines below use relative rounds.

| Timeline | Findings |
|----------|----------|
| **Near term (R162–R166)** | ROB-03 (ownership epoch), ROB-06 (write feedback + cache caveat), MAINT-03 (mirror dedup / gate check), TEST-02 (stash harness) |
| **Medium term (R167+)** | MAINT-05 (dead code), MAINT-07 (naming), FEAT-01 (session journal), FEAT-03 (ladder tripwires), FEAT-04 (restart-only docs), ARCH-01 (gate skeleton standardization), ARCH-02 (mirror resync) |

---

## Architecture Strengths

These patterns are working well and should be preserved during any refactoring.

1. **The marker discipline itself.** 147 `VP-BEGIN` regions across 49 upstream files, each tagged with its issue and round (`[R153]`, `[#37]`), plus fork-only TUs so upstream files never grow new code. This is what makes a 771K-line generated tree forkable at all, and it survived eleven upstream syncs before retirement. The merge tripwires that back it — `static_assert`s on struct size/enum values/`offsetof(D2UIFlagStrc, VanillaPlus)==0x98` plus a runtime `VANILLAPLUS_SelfCheck` that *scans* the wired tables instead of trusting row indices (`VanillaPlusCore.cpp:1719-1832`) — are a model pattern for anyone maintaining code against a generated baseline.

2. **Count-then-refuse patching (`Redivivus/d2_patch.cpp`).** Patches never hardcode 1.14d offsets: `D2PATCH_RedirectAllCalls` scans `.text` for `E8 rel32` targeting a linker-resolved symbol, counts matches against a declared expectation, and **writes nothing on mismatch** (`:165-199`). The design comments are a design essay; keep them.

3. **The 2-second poll-gate charter, uniformly applied.** Fourteen accessors with an identical skeleton, every feature opt-in OFF, missing-ini = byte-vanilla, and a single cache-bust choke point (`D2VP_ConfigReloaded`) every gate registers with. The per-row `nReadDefault` (R144) makes the Options page display what the gate actually acts on — killing the "silent no-op first click" bug class structurally.

4. **Atomic file writes done properly.** The stash write path (temp file → `FlushFileBuffers` → `CopyFileA` one-generation `.bak` → `MoveFileExA(REPLACE_EXISTING)`, `VanillaPlusStash.cpp:419-485`) and the `CREATE_NEW` ini template (`VanillaPlusCore.cpp:343`) are textbook; the load-side bounds checking of `_d2vp_shared.sss` (extent, per-page caps, checksum, `'JM'` verify, short reads, `.bak` chase) is complete.

5. **Choke-point fixing over point fixes.** The #37 crash family ended not by chasing ~30 free sites but by unlinking any item unit at `CUnit::RemoveObject` before `FreeAny` (`CUnit.cpp:1988-2035`) — self-guarding, no-op where already unlinked, covering every current and future free path. The same instinct produced the single save dispatcher as the I1 invariant gate (`PlayerSave.cpp:1129-1134`).

6. **A crash reporter that never touches the heap.** `d2_crash.cpp` formats with `wsprintfA` only, symbolizes on worker threads with a 60 s timeout and an addresses-only fallback, writes via `CREATE_NEW` with suffix rotation and `%TEMP%` then boot-log fallbacks, keeps separate once-slots for own-code vs foreign-module faults, and derives the exe range live at fault time. The init-priority ladder (130 boot → 140 crash → 150 trace → 200 runner) guarantees diagnostics exist before anything that can fault.

7. **Documented concurrency decisions.** The control channel queue-on-socket-thread/apply-on-game-thread split, the deliberately lock-free `ping` (so a wedged game is distinguishable from a wedged channel), and `state`'s documented torn-read trade-off (`d2_control.cpp:234-250`) — unusual care to write the *why* next to each concurrency choice.

8. **The docs system.** The two-lane policy (concise human docs; verbose agent memory with a 16 KB handoff budget and verbatim HISTORY offloading), the SMOKE checklist as the standing field-verification contract, and per-round records going back 78 H-entries — the project has almost no institutional memory locked in anyone's head.

9. **Frame-epoch caching for interpolation.** The unlock-FPS glide computes its delta once per present, keyed on an epoch counter shared by every consumer (`VanillaPlusCore.cpp:1145-1149, 1395-1462`), with a comment explaining why the stats counter can't serve as the epoch (it resets every 5 s window — a former real bug, now documented). Wrap-safe unsigned time math throughout.
