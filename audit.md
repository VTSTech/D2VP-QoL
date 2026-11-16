# Improvement & Enhancement Audit

**D2VP-QoL (Vanilla+QoL) v0.0.8-R158**

**Repository:** https://github.com/VTSTech/D2VP-QoL
**Author:** VTSTech (fork of jaenster/redivivus) | **License:** see LICENSE | **Date:** 2026-11-16 (commit 0951f99)
**7 Findings** | **7 Categories** | SEC, ROB, MAINT, PERF, FEAT, ARCH, TEST

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

The audit produced 31 findings: 2 High, 16 Medium, 13 Low. Both High findings are data-integrity risks in the shared-stash subsystem: a transient read failure of `_d2vp_shared.sss` pins an all-empty in-memory state for the session, and the next save overwrites the good on-disk file (the one-generation `.bak` is the only recourse); and a partially-failed page materialization re-serializes the incomplete set over the full block, permanently dropping the records that failed to decode. The most consequential Medium cluster sits in the DEV-only control channel (`Redivivus/d2_control.cpp`), which binds an unauthenticated TCP listener on 0.0.0.0:7777 with commands for arbitrary memory reads, raw packet injection and arbitrary file writes — correctly compiled out of release builds, but exposed to the local network whenever a DEV build runs. The rest of the Medium set is a mix of silent-failure paths (ini write failures produce no user feedback, and the Windows profile cache can defeat the documented "external editor" hot-read contract), boot-path buffer-formatting limits (`wvsprintfA`'s 1024-char output cap against undersized stack buffers), and documentation drift (`CONTRIBUTING.md` still cites the retired 525-check gate script that no longer exists in `tools/`).

The overriding pattern across the codebase is unusually high discipline: every fork edit is tagged `[R###]`/`[#issue]` between `// VP-BEGIN`/`// VP-END` markers, every feature is opt-in with byte-vanilla defaults, every crash fix lands at a choke point with a written analysis, and the docs encode a deliberate two-lane policy (concise human docs, verbose agent memory). The gaps are correspondingly systematic too: correctness currently rests on the owner's field verification and static re-reads rather than any automated test, and the retired gate suite means marker/doc hygiene checks now only run on files that happen to change.

---

## Findings Summary

| ID | Severity | Category | Title |
|----|----------|----------|-------|
| ROB-03 | Medium | Robustness | Stash ownership pin uses pointer identity; address reuse resurrects stale pool pointers |
| MAINT-03 | Medium | Maintainability | Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair |
| TEST-02 | Medium | Testing | No automated tests for stash serialization / save invariants |
| FEAT-01 | Medium | New Feature | Session journal or save-on-flip for the memory-only personal stash
| SEC-03 | Low | Security | `d2_boot` path assembly with `lstrcpyA`/`lstrcatA` can overrun by ~11 bytes |
| ROB-09 | Low | Robustness | Three unbalanced VP-BEGIN markers; two nested marker pairs |
| ROB-10 | Low | Robustness | Bounded-but-fragile buffer idioms (ilvl suffix, label buffers, walldir slot) |
| MAINT-04 | Low | Maintainability | Stale doc comments (clock format, options geometry, README `__ftol2`) |
| MAINT-05 | Low | Maintainability | Dead code inventory: parked diagnostics, unused macro, `if (0)`, `sym` command |
| MAINT-06 | Low | Maintainability | Hardcoded "0.0.8" literal in boot self-ID drifts from build tag |
| MAINT-07 | Low | Maintainability | Naming warts: `VPSHADOW`, `VPSTASH_PAGES/COUNT`, mixed ScreenSize namespaces |
| PERF-01 | Low | Performance | Options page polls 16 ini gates per frame while open |
| PERF-02 | Low | Performance | `timeBeginPeriod(1)` is a one-way latch, never undone |
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

#### ROB-01: Transient stash read failure pins degraded state; next save overwrites good file

| Property | Value |
|----------|-------|
| **Severity** | High |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:399-414, 419-485` |

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

#### ROB-04: Personal stash is memory-only while a shared page is active

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:160-162, 652` |

#### ROB-09: Three unbalanced VP-BEGIN markers; two nested marker pairs

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:358, 1755-1832` |

#### MAINT-01: CONTRIBUTING cites retired gate script `tools/check-vanilla-plus.sh`

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Robustness |
| **File(s)** | `Source/D2Client/D2Client.cpp:1815`, `Source/D2Client/UI/ui.cpp:8952`, `Source/D2Client/GAME/Wall2.h:63`, `Source/D2Client/UI/Path.cpp:187-225` |

Census: 147 `VP-BEGIN` markers across 49 files. Three files have an open `VP-BEGIN` with no matching `VP-END` (`D2Client.cpp:1815` `[issue #26 / R92]`, `ui.cpp:8952` `[issue-18 fmt-args]`, `Wall2.h:63` `[R118]`), and `Path.cpp:187/199` + `:217/225` contain `VP-BEGIN [R92]` regions nested inside open `VP-BEGIN [R95]` regions. These are hygiene issues today, but `docs_gate.py` §8 checks marker pairing on git-changed files — each will fail the gate the next time its file is touched.

**Recommendation:** close the three markers (or mark the region intentionally-open with a matching comment convention docs_gate understands) and flatten the two nested pairs.

**Impact:** The marker contract — the fork's insurance against source regeneration — stays mechanically verifiable.

---

#### ROB-10: Bounded-but-fragile buffer idioms

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Robustness |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:612-646`, `Source/D2Client/UI/ShowItems.cpp:343-345`, `Source/D2Win/Src/D2WinFont.cpp:1560-1582`, `Source/Fog/Source/SFile.cpp:355, 527`, `Redivivus/d2_control.cpp:632-638` |

A cluster of places where safety holds today but rests on exact-fit or cross-module slack rather than an explicit bound: `ITEMILVL_AppendNameSuffix` appends into an upstream-owned buffer whose capacity it cannot see (`Core.cpp:612-646`, safe only by the documented slack in `Core.h:174-176`); ground-item labels fill `wszName[128]` via a 0x100 **byte** cap with zero slack (`ShowItems.cpp:343-345`); the shift-compare guard is `< 0x400` rather than `<=` (`D2WinFont.cpp:1560-1582`); and two recovered-retail bugs sit in the registry-migration path of `SFile.cpp` — `szSrcPath[nLen + 0x103]` reads an uninitialized local (`:355`, `szDstDir` intended) and `szOldSavePath` is read where `szNewSavePath` appears intended (`:527`).

**Recommendation:** pass explicit capacities into `ITEMILVL_AppendNameSuffix`; add one-element slack or a static_assert to label buffers; fix the two `SFile.cpp` registry-path reads (they are upstream-visible bugs, not fork edits — record upstream or wrap in a VP region).

**Impact:** Removes the handful of spots where an unrelated upstream change would turn a documented invariant into an overflow.

---

### Maintainability

#### MAINT-03: Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:413-453` vs `VanillaPlusCore.cpp:1572-1612`; `Options.cpp:477-591` vs `Core.cpp:1617-1717`; `Source/D2Direct3D` vs `Source/D2Glide` camera-zoom sites (R155/R156) |

### Performance

#### PERF-01: Options page polls 16 ini gates per frame while open

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:218-224`, `:113-119`, `Source/D2Client/UI/VanillaPlusCore.cpp:1072-1075, 2778-2779`, `:403-417` |

Small consistency issues: the checksum helper is named `VPSHADOW_BlockChecksum` (missing "S" for VPSHARED) and also only detects truncation, not bit rot; `VPSTASH_PAGES` vs `VPSTASH_PAGE_COUNT` are both 100 but serve array-size vs file-format roles (colliding names invite a wrong-site edit); `PANELSHIFT_NoShiftWorld` reads `D2Client::UI::ui::ScreenSizeX` but `D2Client::UI::Inv::ScreenSizeY` — two module headers re-exporting presumably the same global, which breaks subtly if they ever diverge (same pattern at `Core.cpp:2778-2779` and `Options.cpp:334-335`); and the alt-toggle latch `gbAltLatched` is never cleared on gate-off (harmless today because every consumer requires both signals, but a future consumer of `ALTTOGGLE_Latched` alone inherits stale state).

**Recommendation:** rename `VPSHADOW`→`VPSHARED`, pick one of the two page-count constants and alias the other, standardize on one ScreenSize re-export, and clear the latch in the gate-off path.

**Impact:** Removes four places where a mechanical-looking edit produces a subtle bug.

---

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
| **Near term (R162–R166)** | ROB-03 (ownership epoch), ROB-07 (dead/trade refusal), ROB-06 (write feedback + cache caveat), MAINT-03 (mirror dedup / gate check), TEST-02 (stash harness) |
| **Medium term (R167+)** | ROB-09 (marker closure), ROB-10 (buffer idioms), MAINT-05 (dead code), MAINT-07 (naming), PERF-01/02, FEAT-01 (session journal), FEAT-03 (ladder tripwires), FEAT-04 (restart-only docs), ARCH-01 (gate skeleton standardization), ARCH-02 (mirror resync) |

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
