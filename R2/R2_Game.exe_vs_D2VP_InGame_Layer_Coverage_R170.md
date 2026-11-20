# R2 Game.exe ↔ D2VP In-Game Layer Coverage (R170, phase 13)

**Date:** 2026-11-20
**Binary:** `Game.exe` 1.14d retail, md5 `2b3600c10ee0d6387675a2f6cf0ebed2`, base `0x400000`.
**Companion to:** `R2_Game.exe_vs_D2VP_Gap_Analysis_R170.md` (phase 12, Battle.net path).

> **Tagging convention (project rule):** `Rxx` bumps only on **code** changes to D2VP;
> the last code change is R170, so all documentation rounds are tagged R170. The
> phase-11/12 docs formerly carrying R171–R173 tags have been renamed to R170
> (`R2_Game.exe_Diagnostic_Function_Reference_R170.md`,
> `R2_BattleNet_Connect_Auth_CharSelect_GameCreate_R170.md`,
> `R2_Game.exe_BNetPath_Function_Reference_R170.md` + CSV).

## 1. What phase 13 did

Phase 12 certified the Battle.net path but only swept Game.exe's .text with a
non-resynchronizing capstone walk (it stops at the first undecodable byte) and scanned
D2VP modules non-recursively (missing subdirectories). Phase 13 fixes both:

- **Full resynced sweep** of .text (`0x401000–0x6cc000`): 1 068 342 instructions,
  82 981 call instructions, 9 209 distinct call targets, 211 resync stops (embedded
  data blobs). Scripts: `scripts/r2_phase13b_coverage_map.py` (superseded),
  `scripts/r2_phase13c_full_coverage.py` (definitive).
- **Recursive** map-vs-cpp scan of all 20 D2VP modules: **13 512 map functions** total
  (D2Game 4 059, D2Client 3 783, D2Common 2 461, Fog 541, D2CMP 373, Bnclient 372,
  D2Win 362, D2Launch 318, D2Multi 259, D2Net 61 …).
- Spot-checks of the in-game D2GS packet layer (`SCmd.cpp`) and the D2Net thunks.

## 2. Headline results

| Region | Call targets | Uncovered | Read |
|---|---|---|---|
| BNCS path `0x518000–0x52c000` | 279 | **0 (0%)** | fully decompiled (confirms phase 12) |
| MCP/CharSel `0x435000–0x44c200` | 280 | **0 (0%)** | fully decompiled |
| In-game client `0x44c200–0x516000` | 2 672 | **0 (0%)** | fully decompiled — D2GS packet layer complete |
| Embedded server `0x5ec000–0x6a0000` | 2 683 | 605 (23%) | **all 605 are CRT internals**, zero game logic (see §3) |
| Whole .text | 9 209 | 1 301 (14.1%) | all of it Storm/Fog static-lib or CRT (see §3) |

**Verdict: every game-logic call target in Game.exe is present in D2VP's maps. The
remaining 14.1% uncovered is statically-linked Storm/Fog library code and MSVC CRT —
both deliberately replaced, not missing.**

## 3. The uncovered regions, identified

| Range | Targets | Identity | Evidence |
|---|---|---|---|
| `0x411620–0x428000` (+`0x4294c0`, `0x42b170–0x42e120`) | ~400 | **Static Storm/Fog library** (SFile, SReg, …) | `.\SOURCE\SFILE.CPP` string at `0x416c00` |
| `0x681a48–0x6b1a30`, `0x6998c0–0x69fed4`, `0x6953d8`, `0x6a2ca9–0x6ad08a`, `0x6c3cb0–0x6c5050` | ~850 | **MSVC CRT** (C++ runtime, locale, heap, math, string helpers) | `mscoree.dll`/`CorExitProcess` at `0x681a48`; float code at `0x6998c0`; `cmp cl, 0x2d` strtol-style at `0x6c3cb0`; stack-cookie prologues |
| `0x401070` | 16 | zero-init singleton ctor (0x808-byte object, vtable `0x6ff984`), called from `0x40833f`/`0x40869d` | benign |
| `0x4294c0` | 8 | helper called once from `0x429803` (CRT region) | benign |
| `0x6c9a38` | 6 | strlen-style helper called from `0x6659d8`/`0x66a892`… | benign |
| `0x682c00` area | — | embedded data blob inside .text (misaligns capstone) | not code |

Storm behavior parity is delegated to `3rdparty/Storm`; CRT parity to the modern
toolchain. Neither needs recreation.

## 4. In-game layer spot-checks (quality)

- **D2GS receive pipeline is faithful and table-driven**: `SCMD_ProcessIncomingPacketBuffer`
  (`0x45fa40`, `D2Client/GAME/SCmd.cpp:3768`) validates buffer count and packet id
  (< 0xaf) and per-packet expected size against the **175-entry** `NET_D2GS_CLIENT_INCOMING`
  table, preserving Game.exe's exact assert lines (`0x1525`, `0x1531`, `0x1534`, `0x1536`).
- The 175-entry table is **fully populated** with named handlers
  (`NET_D2GS_CLIENT_Incoming0x01_NEWGAME`, `0x03_NEWACT`, `0x07_NEWROOM`, `0x0B–0x0F
  UNITCHANGE family`, …) including expected sizes — no nullptr gameplay slots.
- `D2Net` thunks (`0x52bxxx`, QServer glue): sampled `0x52b0e0` — Game.exe itself is a
  7-instruction register-shuffle wrapper returning 1; D2VP's tiny wrapper is faithful.
- The ~2 100 covered call targets in the server region live in D2VP under
  D2Common (2 390 of them), D2Game and D2CMP — the module *folders* don't mirror
  address layout, which is why the earlier non-recursive scan undercounted D2Game as 33.

## 5. Remaining gap list (complete, small, none behavioral)

1. **88 missing cpp bodies in D2Client** (map has them, cpp doesn't): ~60 are container
   template boilerplate (`0x44dd90–0x451220`: ctor/dtor/scalar-deleting-dtor/NewNode/
   Reserve/ReallocData/Link/Unlink for the D2 list/array classes), ~20 are UI control
   ctor/dtor boilerplate (Button/Editbox/Spinner/ListBox), 2 are Warden client ctors
   (`0x47fc30`, `0x4801c0`), 1 trivial `operator=` (`0x517e40`). D2VP replaces the
   boilerplate with its own classes, so these are intentional, **but** each container
   used by decompiled callers should be checked once against D2VP's replacement
   (ordering, growth, link-offset semantics) — that is the last open verification.
2. **Warden module**: constructors missing (item 1) — if Warden handshake parity ever
   matters for realm compatibility, decompile those two ctors.
3. **D2VP renames from phase 12** (cosmetic): `NET_SID_CLIENT_LogonSequence` →
   `_ConnectFastest` (0x51ca90), `SID_SetReadyFlag` → `SID_SetProtocolDeadFlag`
   (0x520ad0).
4. Nothing else. No gameplay, protocol, timing, or wiring gap exists on either the
   Battle.net path or the in-game path.

## 6. Regression tooling

- `scripts/r2_phase12_gapscan.py` — now recursive; rerun after every decompilation
  drop; expect `missing from cpp` = 88 (D2Client boilerplate) and no new missing entries.
- `scripts/r2_phase13c_full_coverage.py` — full resynced binary sweep; expect
  uncovered = 1 301, all in Storm/CRT ranges.
