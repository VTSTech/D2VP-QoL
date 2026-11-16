# Improvement & Enhancement Audit

**D2VP-QoL (Vanilla+QoL) v0.0.8-R158**

**Repository:** https://github.com/VTSTech/D2VP-QoL
**Author:** VTSTech (fork of jaenster/redivivus) | **License:** see LICENSE | **Date:** 2026-11-16 (commit 0951f99)
**31 Findings** | **7 Categories** | SEC, ROB, MAINT, PERF, FEAT, ARCH, TEST

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
| ROB-01 | **High** | Robustness | Transient stash read failure pins degraded state; next save overwrites good file |
| ROB-02 | **High** | Robustness | Partial page materialization re-serializes incomplete set, dropping records |
| SEC-01 | Medium | Security | DEV control channel binds unauthenticated listener on 0.0.0.0 |
| SEC-02 | Medium | Security | Control-channel commands parse remote input unsafely (deref, negative index) |
| ROB-03 | Medium | Robustness | Stash ownership pin uses pointer identity; address reuse resurrects stale pool pointers |
| ROB-04 | Medium | Robustness | Personal stash is memory-only while a shared page is active |
| ROB-05 | Medium | Robustness | `wvsprintfA` 1024-char output cap overruns undersized boot/crash log buffers |
| ROB-06 | Medium | Robustness | Failed ini writes are silent; Windows profile cache defeats the hot-read contract |
| ROB-07 | Medium | Robustness | Stash header claims dead/trading refusal that is not implemented |
| ROB-08 | Medium | Robustness | `VANILLAPLUS_SelfCheck` calls `exit(-1)` mid-game on first ini poll |
| MAINT-01 | Medium | Maintainability | CONTRIBUTING cites retired gate script `tools/check-vanilla-plus.sh` |
| MAINT-02 | Medium | Maintainability | Adding a feature requires 4 coordinated edits; in-code comment understates it |
| MAINT-03 | Medium | Maintainability | Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair |
| ARCH-01 | Medium | Architecture | 2-second ini-gate pattern duplicated across the module boundary |
| TEST-01 | Medium | Testing | Gate suite retired; marker hygiene now only checked on git-changed files |
| TEST-02 | Medium | Testing | No automated tests for stash serialization / save invariants |
| FEAT-01 | Medium | New Feature | Session journal or save-on-flip for the memory-only personal stash |
| FEAT-02 | Medium | New Feature | User-visible feedback for failed settings writes |
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

#### SEC-01: DEV control channel binds unauthenticated listener on 0.0.0.0

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Security |
| **File(s)** | `Redivivus/d2_control.cpp`, `Redivivus/d2_control.h`, `CMakeLists.txt` |

The in-process dev control channel (`D2CONTROL_PORT` 7777, `d2_control.cpp:92`) binds `INADDR_ANY` with `SO_REUSEADDR` (`d2_control.cpp:1702-1715`) and is explicitly unauthenticated (documented in `d2_control.h:9` and the CMake option text). Its command surface is powerful: `hex <addr> <len>` reads up to 1024 bytes from any address (`:462-483`), `shot <path>` writes a screenshot to any path (`:1663-1675`), `cpkt <hex>` sends arbitrary C→S packets through the game's own sender (`:1599-1629`), `watch`/`guard` retarget debug registers and flip page protections (`:1229-1266`, `:1728-1746`), and `buy`/`shoptx` drive purchases. Any host on the network can connect.

Mitigations verified: the entire TU is **removed from the build** when `REDIVIVUS_DEV=OFF` (`list(REMOVE_ITEM D2_SOURCES ...)` in `CMakeLists.txt`, not an `#ifdef`), the header stubs compile to nothing, and `BUILD_KIND` changes to "DEV" in the crash report — so a release binary cannot contain the listener. The residual risk is operational, not shipped code.

**Recommendation:** bind `127.0.0.1` instead of `INADDR_ANY` (loopback preserves the entire debugging workflow — a local agent connects the same way), and/or print the bind address prominently in `d2boot.log` on DEV builds. One-line change, removes the whole network-exposure class.

**Impact:** Eliminates the possibility of a remote peer driving a DEV build's memory reader, packet injector and screenshot writer, at zero cost to the debugging workflow.

---

#### SEC-02: Control-channel commands parse remote input unsafely

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Security |
| **File(s)** | `Redivivus/d2_control.cpp` |

Three input-handling defects in the same DEV-only command surface:

1. `watch <hexaddr>` (`d2_control.cpp:1229-1243`) sets `g_pHwbpTarget` from remote input; the reply immediately dereferences it — `*(unsigned*)(uintptr_t)g_pHwbpTarget` — with **no** `IsBadReadPtr` guard (unlike the `hex` command). A remote `watch 1` faults the process. The unguarded target is then passed to `VirtualProtect` by `guard`/`D2ProbeArmClipGuard` (`:1728-1746`).
2. `mon <start> <count>` (`:254-273`) takes `sscanf` values without checking the return; a negative `nStart` passes the loop condition and indexes `pTxtMonStats[i]` with a negative index — out-of-bounds read → AV.
3. Several other commands (`ai`, `setclip`, `walldir`'s `%x` slot at `:632-638`, `objname`) consume unchecked `sscanf` values; `walldir`'s `kSlot` offsets past the `IsBadReadPtr`-checked range.

The parse framework itself is sound (1024-byte line buffer, drop on overflow, bounded reply accumulation), so these are localized per-command fixes.

**Recommendation:** add `IsBadReadPtr` before the `watch` reply dereference; check every `sscanf` return and clamp `nStart`/`kSlot` to their table bounds. All three fixes are under 10 lines total.

**Impact:** A malformed debug command can no longer crash the exact process the channel exists to debug.

---

#### SEC-03: `d2_boot` path assembly with `lstrcpyA`/`lstrcatA` can overrun by ~11 bytes

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Security |
| **File(s)** | `Redivivus/d2_boot.cpp:26, 36, 40` |

Boot-log and crash-report path construction appends fixed suffixes (e.g. `d2boot.log`, timestamped crash names) after a `GetModuleFileNameA` result that may already be `MAX_PATH-1` long. `lstrcpyA`/`lstrcatA` perform no bounds checking; the overflow potential is ~11 bytes on an exe placed at a near-260-char path. Low likelihood (requires an extremely deep install path) but on the boot path where nothing catches it.

**Recommendation:** use the same bounded-copy idiom the fork already uses elsewhere (`SStrCopy`/manual cap, as `ALTTOGGLE_BuildIniPath` does with its 240-char cap at `VanillaPlusCore.cpp:163-176`).

**Impact:** Removes a small stack-overrun on the earliest-executing code in the process.

---

### Robustness

#### ROB-01: Transient stash read failure pins degraded state; next save overwrites good file

| Property | Value |
|----------|-------|
| **Severity** | High |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:399-414, 419-485` |

`VPSHARED_EnsureLoaded` sets `gbSharedLoaded=1` **before** knowing the read outcome (`:399-403`). If the first toggle happens while the file is transiently locked or unreadable, the session runs on an all-empty in-memory state — and the next toggle-to-personal or save serializes that empty state over the good on-disk file (`:419-485`). The one-generation `.bak` (`:478`) preserves the pre-wipe data, but pages and gold beyond what the current session touched are lost until a manual `.bak` restore, which no user knows to do.

The read-side validation itself is genuinely excellent — magic+version+page-count checks (`:330-332`), header-vs-filesize extent check (`:351`), per-page bounds + block cap + item cap (`:242`, `:279`), checksum, `'JM'` verify, short-read detection, and a `.bak` retry on page-1 failure (`:407-413`) — a misread never destroys the file directly. The hole is specifically the transient-failure-at-first-load path.

**Recommendation:** distinguish "loaded successfully" from "load attempted" (e.g. `gbSharedLoaded=1` only on success; on transient failure retry on the next toggle instead of pinning the empty state for the session), and refuse to write when the last read failed rather than succeeded-with-empty.

**Impact:** Protects the shared stash — the feature holding the most accumulated player value — from silent wipe by a transient lock.

---

#### ROB-02: Partial page materialization re-serializes incomplete set, dropping records

| Property | Value |
|----------|-------|
| **Severity** | High |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:591-595, 692-693, 721-722` |

When a shared page is materialized into live item units, each record is decoded via `PLRSAVE_DispatchItemLoadByFormat` (`:591`). If any record fails to decode partway, the page's in-memory set is incomplete (marked "partial page"). The next page flip or save re-serializes that partial set over the full stored block (`:692-693`, `:721-722`) — the records that failed to decode are permanently dropped, silently. The swap-in-only-on-success pattern (`:683-684`, `:712-714`) protects against building a *new* block, but not against committing an *impoverished* one.

**Recommendation:** on a partial materialization, keep the original serialized block as the authoritative copy for the affected page (write-through skip), surface a `D2BOOT_Mark` line, and force a `.bak` rotation before any overwrite. Alternatively, only commit the re-serialized block if the record count matches the source count.

**Impact:** Guarantees that a decode failure degrades one session instead of erasing items permanently.

---

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

Activating a shared page frees the personal item units (`:652`); the only copies are the serialized block and gold snapshot in BSS (`:160-162`). Any exit that never runs `SaveGameAllGameTypes` — crash, forced kill, power loss — destroys the personal stash set and personal gold deposited before the toggle. This is the designed I1 trade-off, but the blast radius is the player's *entire* personal inventory, and nothing warns the user.

**Recommendation:** at minimum, document the exposure in README/FEATURES (it is currently invisible); better, see FEAT-01 for a save-on-flip mitigation.

**Impact:** Converts a silent total-loss edge into a known, documented (or mitigated) one.

---

#### ROB-05: `wvsprintfA` 1024-char output cap overruns undersized boot/crash log buffers

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Redivivus/d2_boot.cpp:50-51`, `Redivivus/d2_crash.cpp:192`, `Redivivus/d2_trace.cpp:73-77` |

`wsprintfA`/`wvsprintfA` truncate at **1024 output characters** — they accept no destination-size parameter. `BootLogWrite` formats into `szBuf[640]` and `D2BOOT_MarkFmt` into `szBody[512]`: a long crash-report line (`s_szLine[1100]` → `D2BOOT_Mark`) or a long `D2BOOT_Fail` message (`szMsg[1024]` → `BootLogWrite`) can overrun the stack buffer by up to ~390 bytes — precisely on the "process is dying" path, where `g_nBusy` suppresses the secondary fault and the report silently corrupts. The comment at `d2_crash.cpp:178-179` ("Lines longer than ~600 chars are truncated by the boot writer") describes intent, not actual API behavior. The authors demonstrably know the trap — `d2_hangwatch.cpp:216-224` pre-truncates symbol names for exactly this reason — but the boot writer itself does not enforce it.

**Recommendation:** pre-truncate all inputs to `BootLogWrite`/`D2BOOT_MarkFmt` below the destination size (a shared `D2BOOT_AppendTrunc(dst, cb, src)` helper), and fix the stale comment.

**Impact:** Crash and boot diagnostics — the project's primary field-data channel — become trustworthy on the paths where they matter most.

---

#### ROB-06: Failed ini writes are silent; Windows profile cache defeats the hot-read contract

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:1479-1513`, `Source/D2Client/UI/VanillaPlusOptions.cpp:684-685` |

Two related issues on the settings path:

1. `D2VP_SetFeatureToggle` deliberately ignores the `WritePrivateProfileStringA` return (`Core.cpp:1511-1512`), and `VANILLAPLUS_ToggleRow` unconditionally plays the select sound (`Options.cpp:684-685`). On a read-only game directory the toggle appears to work, then the On/Off display snaps back on the next poll. There is no feedback path.
2. After the first in-process write, Windows serves subsequent `GetPrivateProfileIntA` calls from the process profile cache; external edits to `d2vp.ini` may stop being seen. Every gate comment promises "2 s hot-read ... exactly like an external editor" (e.g. `Core.cpp:1475-1477`) — after the first in-game toggle, that promise can silently break. Worth a verification test; if confirmed, it needs a documented caveat or a cache-busting workaround.

**Recommendation:** check the write result and surface failure (log line + distinct sound or a brief on-screen mark); verify the cache behavior and document it in the header and ini template if real.

**Impact:** Users on read-only installs get truthful feedback, and the documented hot-reload contract matches observed behavior.

---

#### ROB-07: Stash header claims dead/trading refusal that is not implemented

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.h:56-58`, `:742-781` |

`VanillaPlusStash.h` states the shared stash "Refuses while the player is dead or trading", but neither `VPSHARED_ShouldInterceptClickButton` (`:742-759`) nor `VPSHARED_OnClickButton` (`:761-781`) checks dead or trading state. Button 0x7D is intercepted before all retail trade logic, so a page toggle during an active trade is processable. Exploitability is low (the SP client is the only sender), but the header is the contract a developer reads first.

**Recommendation:** either implement the two checks (a few lines reading player state before accepting 0x7D) or fix the header comment. Implementing is preferable — page flips mid-trade also interact oddly with the trade window's item display.

**Impact:** Restores the documented invariant (or removes the false claim), closing a small logic trap in trade scenarios.

---

#### ROB-08: `VANILLAPLUS_SelfCheck` calls `exit(-1)` mid-game on first ini poll

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Robustness |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:358, 1755-1832` |

The fork's runtime tripwire — `static_assert`s plus `VANILLAPLUS_SelfCheck`, which scans the wired options tables for the registered callback instead of trusting row indices — is an outstanding pattern. But it runs exactly once, at whatever first `ALTTOGGLE_EnsureIni` poll triggers it (`:358`), which can be deep inside gameplay UI handling; any tripped assert calls `exit(-1)` there. Intentional ("fail loudly"), but the failure mode is a mid-game process death that looks identical to a random crash to a user who didn't build the exe.

**Recommendation:** run the self-check once at boot (it is already called from boot; make that the only entry point, or add a boot-time force-run) so a tripped tripwire produces a clean `d2boot.log` diagnosis before any game state exists.

**Impact:** A developer-misconfiguration becomes a boot-time diagnostic instead of a field crash report.

---

#### ROB-09: Three unbalanced VP-BEGIN markers; two nested marker pairs

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

#### MAINT-01: CONTRIBUTING cites retired gate script `tools/check-vanilla-plus.sh`

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Maintainability |
| **File(s)** | `CONTRIBUTING.md` ("the gate suite ([`tools/check-vanilla-plus.sh`](tools/check-vanilla-plus.sh), currently **525 checks**)"), `tools/` (file absent) |

The file does not exist in `tools/` (verified by listing). Per `handoff.md` §8, the 525-row script was retired at rev 152 with the upstream merge line, and the 16 KB handoff cap "stays the working rule". A new contributor following CONTRIBUTING's house rules will look for the enforcing script, not find it, and won't know which of the seven listed rules are mechanically checked (answer today: none of them — only `docs_gate.py`'s ten checks and the clean-build gate remain).

**Recommendation:** update CONTRIBUTING to name what actually enforces each rule today (`docs_gate.py` §8 pairing/§9 balance, `check-build.sh`, owner build), and either restore a minimal `check-vanilla-plus.sh` wrapping the surviving checks or state plainly that the suite is retired to git history.

**Impact:** New contributors aren't sent chasing a script that no longer exists, and the enforcement surface is honest.

---

#### MAINT-02: Adding a feature requires 4 coordinated edits; in-code comment understates it

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:149-153, 154-265`, `Source/D2Client/UI/VanillaPlusCore.cpp:373-391, 183-351, 1534-1549` |

A new feature touches: (1) the `gaVanillaPlusRows` table, (2) a gate accessor in `VanillaPlusCore.cpp`, (3) a template block in `ALTTOGGLE_WriteIniTemplate`, (4) a bust line in `D2VP_ConfigReloaded` — with the accessor's tick static declared **above** `:1534` for the bust line to reference. Missing (4) yields an Options toggle that doesn't take effect for ≤2 s or appears broken; missing (3) is invisible. The comment at `Options.cpp:149-153` says "+1 entry here and +1 template block — nothing else", which is false for every R147+ feature. The per-feature comments mostly catch it, but the structure is one refactor away from a silent miss.

**Recommendation:** fix the "nothing else" comment; better, make the accessor register itself with `D2VP_ConfigReloaded` (a tiny array of `{static DWORD*, name}` populated by each accessor on first call) so edit (4) disappears structurally.

**Impact:** The most frequently performed fork task — adding a feature — loses two of its four failure modes.

---

#### MAINT-03: Maintained mirror copies: DC6 builder, font metrics, D3D/Glide camera-zoom pair

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:413-453` vs `VanillaPlusCore.cpp:1572-1612`; `Options.cpp:477-591` vs `Core.cpp:1617-1717`; `Source/D2Direct3D` vs `Source/D2Glide` camera-zoom sites (R155/R156) |

Three deliberate duplications, each documented in-code as "keep in sync byte-for-byte": the blank-DC6 placeholder builder and the font-measure/probe logic exist as near-identical twins in `Options.cpp` and `Core.cpp` (rationale at `Core.cpp:1555-1562`); and the camera-zoom feature is a maintained mirror pair across the D3D and Glide renderers, each site cross-referencing the other. Every future zoom site must land twice; the largest of the three is an ongoing divergence risk. The duplication exists for real module-boundary reasons (see ARCH-01), but nothing *tests* the sync.

**Recommendation:** where both twins live in the same module (Options/Core are both D2Client), extract a shared TU; for the renderer pair, add a docs_gate check that diffs the two marker sets and flags a site count mismatch.

**Impact:** Mirror code stays synchronized mechanically instead of by reviewer memory.

---

#### MAINT-04: Stale doc comments

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Maintainability |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:315-319, 2568-2602`, `Source/D2Client/UI/VanillaPlusCore.h:389-391`, `Redivivus/README.md` §A |

Three doc/code drifts: the ini template still describes the automap clock as "two lines ... System (HH:MM 24-hour)" while the shipped R148 implementation is one line in 12-hour form (`Core.cpp:315-319` vs `:2568-2602`); `Core.h:389-391` describes the Options page as "two columns of SEVEN (box 0x151 -> 0x171, back row 0x124 -> 0x144)" while the code is two columns of eight, box 0x191, back row 0x164 (`Options.cpp:142,146,268`); and Redivivus README §A says `__ftol2` is left undefined while `d2_crt_impl.cpp:31-46` now implements it in inline asm.

**Recommendation:** three small comment fixes; consider a docs_gate grep that flags the geometry constants appearing in both header prose and code.

**Impact:** The headers — which developers treat as contracts — match the code.

---

#### MAINT-05: Dead code inventory

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Maintainability |
| **File(s)** | `Redivivus/d2_control.cpp:1950-2120`, `Redivivus/d2_trace.cpp`, `Source/D2Client/UI/VanillaPlusOptions.cpp:70`, `Source/D2Client/UI/VanillaPlusCore.cpp:575` |

Verified-uncalled code (repo-wide grep): `ClipArmerThread` (`:2109`), `ClipWatchThread` (`:2120`), `DumpAllThreadsAt` (`:2069`), the complete IAT hook set `D2ProbeInstallKernelWriteHooks` + four hooks + `PatchIatEverywhere` (`:1950-2064`), and `D2ProbeArmClipGuard`/`D2ProbeRearmClipGuard` (`:1728-1746`) — parked diagnostics from the gnTileClipYMin hunt; `if (0) wsprintfA(...)` at `:961`; the documented-but-unimplemented `sym` command (`:460-462`); `D2TRACE_WatchWrite`'s `SetThreadContext(GetCurrentThread())` DR0 install, which the codebase itself documents as not working on a running thread (`d2_trace.cpp:100-113` vs `d2_control.cpp:1788-1790`). In the fork TUs: unused `#define DEFAULT_SECURITY_COOKIE` (`Options.cpp:70`).

**Recommendation:** either delete the parked diagnostics (git history preserves them — the project already demonstrated this with the sync-tooling retirement) or move them behind a documented `#if 0` block with a one-line reason; delete the unused macro and the `if (0)` statement.

**Impact:** The control-channel surface an auditor must read shrinks by ~250 lines, and none of it is a false friend.

---

#### MAINT-06: Hardcoded "0.0.8" literal in boot self-ID drifts from build tag

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Maintainability |
| **File(s)** | `Source/Game/Main.cpp:1152`, `Redivivus/d2_boot.h:14` |

The boot self-ID composes `D2BOOT_Mark("Vanilla+0.0.8-" D2VP_ROUND)` — the `0.0.8` is a string literal, while the title screen and automap use `D2VP_BUILD_TAG` = `"D2VP-R" D2VP_ROUND` from the same header. Two identity formats for the same build in two places; the literal version prefix will silently drift from CHANGELOG reality at the next release bump (0.0.9) since only one of the two sites is touched by `round_bump.py`'s round census.

**Recommendation:** compose one `D2VP_VERSION_TAG` ("0.0.8-R" D2VP_ROUND) in `d2_boot.h` and use it in both places; add a docs_gate check that the release number appears exactly once.

**Impact:** Every `d2boot.log` line-1 self-ID stays pinned to the actual release without manual double-editing.

---

#### MAINT-07: Naming warts

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

### New Features

#### FEAT-01: Session journal or save-on-flip for the memory-only personal stash

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | New Feature |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp:160-162, 652` (see ROB-04) |

Grounded in ROB-04: while a shared page is active, the personal set exists only as a serialized block in BSS, and a hard exit destroys it. A cheap mitigation that fits the existing architecture: on every toggle-to-shared, write the current personal set + gold snapshot to a small session journal file (same temp+flush+replace+bak discipline the stash already implements at `:419-485`), deleted on clean save. Crash recovery then offers "restore personal set from journal" instead of total loss. The pieces (atomic write, retail record serialization, `.bak` rotation) all already exist in this TU.

**Impact:** Caps the worst data-loss scenario in the feature set at one page-flip interval instead of the whole session.

---

#### FEAT-02: User-visible feedback for failed settings writes

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | New Feature |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:671-686`, `VanillaPlusCore.cpp:1502-1513` (see ROB-06) |

Grounded in ROB-06's silent-failure path: the row toggle plays the select sound unconditionally and the write result is discarded. A minimal feedback feature — on `WritePrivateProfileStringA` failure, play the "won't fit" sound (already used by the ctrl-click transfer for a full target, so the sound vocabulary exists) and write one `D2BOOT_MarkFmt` line — gives read-only-install users truthful feedback with two lines of code. A fuller version draws the row label in red for one second.

**Impact:** The Options page stops lying to users whose game directory is not writable.

---

#### FEAT-03: Compile-time tripwires tying resolution-ladder constants together

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | New Feature |
| **File(s)** | `Source/D2gfx/D2GfxHelperStrc.cpp:41-56`, `Source/D2Client/UNIT/CUnit.cpp:2500-2546`, `Source/D2Client/UI/VanillaPlusCore.cpp:1857-1882` |

Grounded in the issue-#31 history (a static row-table overread fixed at R138 by resizing `gaYBufferRowOffsets` to 1112) and the audit's observation that the sizing is a bare literal: the fork already uses `static_assert` tripwires brilliantly elsewhere (`Core.cpp:1719-1747`). The same treatment for the ladder — a named `VPRES_MAX_HEIGHT` (1080), `static_assert(sizeof(gaYBufferRowOffsets)/sizeof(...) == VPRES_MAX_HEIGHT + 0x20)`, and named constants for the 400/800 panel-space literals in `CUnit.cpp` — makes the next taller resolution a compile error instead of a reopened field crash.

**Impact:** The highest-past-crash-count subsystem gets the same mechanical protection as the settings tables.

---

#### FEAT-04: Hot-reload parity (or documentation) for restart-only settings

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | New Feature |
| **File(s)** | `Source/D2Client/UI/VanillaPlusOptions.cpp:110-111, 538-591` |

`[ui] label_font` is read once per process while every other setting hot-reloads — discoverable only from a code comment, and the ini template doesn't mark it. Either route `[ui]` reads through the same 2 s poll shape (the font probe is expensive, so a "re-decide on change" variant would be needed), or — cheaper — mark the key's comment in `ALTTOGGLE_WriteIniTemplate` with "requires restart" and add a row note on the Options page.

**Impact:** The settings system has one consistent, documented contract instead of a silent exception.

---

### Architecture

#### ARCH-01: 2-second ini-gate pattern duplicated across the module boundary

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Architecture |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:1938-1952`, `Source/D2Game/PLAYER/VanillaPlusStash.cpp:178-191`, `Source/D2Game/UNIT/VanillaPlusCombat.cpp:23-36`, `Source/Fog/Source/SFile.cpp:414-435`, `Source/D2gfx/D2GFX.cpp:998-1022` |

The identical 2-second poll-gate skeleton (static cache + static tick + `GetPrivateProfileIntA` + exe-anchored ini path) is hand-copied in at least five TUs across three modules. The duplication is *forced* by a real architectural constraint — D2Game/D2gfx cannot import D2Client helpers, and the recovered module graph is enforced at build time — so a naive "extract a helper" recommendation is wrong. But the copies have already drifted microscopically (`VANILLAPLUS_GlobeTextPersistent` at `Core.cpp:575` omits the `!lastCheck ||` first-poll guard every other gate has), which is exactly how forced duplication decays. There are also five independent constructions of the exe-anchored ini path (`ALTTOGGLE_BuildIniPath` and equivalents).

**Recommendation:** don't fight the module boundary; standardize instead — a canonical comment block (the "row-28 pattern" doc) stating the exact skeleton, plus a docs_gate check that greps each gate's first-poll guard shape. For the ini path, one correct bounded construction per module is acceptable; verify each matches the 240-cap idiom.

**Impact:** Forced duplication stays honest: drift becomes a gate failure instead of a behavior divergence.

---

#### ARCH-02: Client shared-page mirror drifts from authoritative server page by design

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Architecture |
| **File(s)** | `Source/D2Client/UI/VanillaPlusCore.cpp:1909-1926`, `Source/D2Game/PLAYER/VanillaPlusStash.cpp:761-781` |

The client tab's `gnVpSharedPage` is a cosmetic mirror; the server is authoritative and idempotent. In non-single-player contexts, the client label toggles but the server refuses the 0x4F/0x7D toggle — a documented v1-scope desync (`Core.cpp:1917-1921`). The architecture is sound for SP, but any future work that adds server validation or LAN play will trip on the mirror (a developer flipping the label expects it to reflect server state).

**Recommendation:** when the server handles a 0x7D toggle (accept *or* refuse), echo the effective page in the existing 0x9C stream so the client mirror resyncs; until non-SP matters, a one-line comment at the mirror declaration pointing at the desync doc suffices.

**Impact:** The client/server page-state relationship becomes self-healing instead of developer-knowledge-only.

---

### Testing

#### TEST-01: Gate suite retired; marker hygiene now only checked on git-changed files

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Testing |
| **File(s)** | `tools/` (check-vanilla-plus.sh absent), `tools/docs_gate.py`, `CONTRIBUTING.md` |

The 525-check gate suite that once mechanically enforced most house rules was retired with the upstream merge line (rev 152; script exists only in git history). What remains: `docs_gate.py` (10 checks — round identity, row-table invariants, §8 VP-marker pairing **on git-changed files only**, §9 brace balance, FEATURES row-count parity, overlay verification) and `check-build.sh` (clean-clone link gate). Consequence: the three unbalanced VP markers (ROB-09) are invisible until someone touches those files, and rules 1–5 of the charter are now enforced by convention only. The owner's Win11 build is the compile gate of record, which catches compile breaks but none of the hygiene classes.

**Recommendation:** revive a reduced suite: `docs_gate.py` already contains the pairing check — run it in whole-repo mode (it appears to be diff-scoped for speed) or add a small `check-vanilla-plus.sh` that calls `docs_gate.py --all` plus the chartered grep checks (gate-first-poll shape, LF normalization, marker census). ~30 minutes of work restoring most of the retired coverage.

**Impact:** The house rules return to being mechanically enforced before each overlay ships.

---

#### TEST-02: No automated tests for stash serialization / save invariants

| Property | Value |
|----------|-------|
| **Severity** | Medium |
| **Category** | Testing |
| **File(s)** | `Source/D2Game/PLAYER/VanillaPlusStash.cpp` (whole file) |

The shared stash is the highest-stakes fork subsystem (player item data, file format, atomic writes) and the only one with a bespoke binary format (`'VPS2'` header, block table, checksums, v1→v2 migration) — and it has zero automated tests. The load-side validation is genuinely complete (bounds, caps, checksums, short reads — verified in this audit), but nothing prevents a future format edit from breaking the round-trip. The R117 incident already proved host-side test harnesses can compile *into* the game when parked in the tree (the `scripts/` exclusion in `vanilla-plus/cmake/sources.cmake:33-40` now guards that).

**Recommendation:** a host-side harness under `scripts/` (already excluded from the game link) that exercises `VPSHARED` serialization round-trips against golden bytes: valid v2 load, v1 migration, truncated file, oversized page count, bad checksum, corrupt `'JM'`, and the ROB-01 transient-failure path. The functions are file-serializing and pool-tied, so the harness needs small stubs — the same approach as the R158.3 mock link check.

**Impact:** The format that holds players' accumulated items gets regression protection before the next format-touching change.

---

#### TEST-03: Ad-hoc link checks not committed; no CI artifact test

| Property | Value |
|----------|-------|
| **Severity** | Low |
| **Category** | Testing |
| **File(s)** | `scripts/mock_link_check.cpp` (referenced in `handoff.md` R158.3, not present in tree), `.github/workflows/build.yml` (badge only) |

The R158.3 compile fix used a "mock link check at scripts/mock_link_check.cpp" per handoff — but `scripts/` is not in the tree, so the technique (link fork TUs against stub globals to catch compile breaks before the owner's build) is lost each round. The GitHub Actions build badge exists, but the CI gate isn't described in-tree, and the overlay workflow ships changed-file zips without an artifact-level check beyond `overlay.py`'s byte-identity verification.

**Recommendation:** commit the mock link harness under `scripts/` (already excluded from the game glob) and add one CI step that builds it; document in `docs/building.md` what the CI workflow actually runs versus what stays owner-side.

**Impact:** The compile-fix technique survives rounds, and contributors know which gates run where.

---

## Priority Matrix

The project versions by code round (R###), currently R158. Timelines below use relative rounds.

| Timeline | Findings |
|----------|----------|
| **Near term (R159–R161)** | ROB-01 (stash degraded-read wipe), ROB-02 (partial materialization loss), SEC-01 (loopback bind), SEC-02 (control-channel input guards), ROB-05 (boot-log truncation), MAINT-01 (CONTRIBUTING gate reference) |
| **Short term (R162–R166)** | ROB-03 (ownership epoch), ROB-07 (dead/trade refusal), ROB-06 (write feedback + cache caveat), MAINT-02 (feature checklist structural fix), TEST-01 (revive reduced gate), TEST-02 (stash harness), FEAT-01 (session journal), FEAT-02 (failed-write feedback), ROB-08 (self-check at boot) |
| **Medium term (R167+)** | MAINT-03 (mirror dedup / gate check), ROB-09 (marker closure), ROB-10 (buffer idioms), MAINT-04 (stale comments), MAINT-05 (dead code), MAINT-06 (self-ID composition), MAINT-07 (naming), PERF-01/02, FEAT-03 (ladder tripwires), FEAT-04 (restart-only docs), ARCH-01 (gate skeleton standardization), ARCH-02 (mirror resync), TEST-03 (committed link harness), SEC-03 (boot path bounds) |

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
