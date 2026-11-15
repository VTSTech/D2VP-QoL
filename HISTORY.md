# HISTORY — the port effort (D2VP-QoL fork → redivivus port branch)

This file previously held the fork's Vanilla+ history (H1–H75, preserved
verbatim on `origin/main`); it is repurposed as the port's chronological log.
R-numbers below refer to fork feature rounds; port sessions are P-numbered.

## P1 — Plan and inventory

Port plan drafted (`port-plan.md`): upstream redivivus base + all 53
VP-marked files inventoried. Phases 1–5 completed up front: VP core files
added (VanillaPlusCore/Options/Stash/Combat/Sound, client + D2Game sides),
VP-marked changes applied across D2Client/D2Game/renderers/globals, support
files (`d2_boot.h`, `d2_hangwatch.h`) and docs carried over.

## P2 — Build Round 1 (first configure + compile)

mingw-i686 toolchain wired into CMake; ~450+ of 500+ TUs compiling (90%).
Fixed: module gates (D2_GATE_FROM_ROOT in globals.h), fork typedefs added
(`fpProfCoreInitCb`, `gnGlideWindowHandle`, `gbBloodFlag`, …), conflicting
typedefs removed, D2GFX.h struct guards, signature mismatches (D2GFX_CreateWindow
HWND→BOOL, SkillNec, SUnit::CreateUnit), missing decls (SOUND_GetDirectSoundInitializedFlag,
D2TCStackEntryStrc members). Remaining ~15 errors concentrated in Gfx.cpp /
Sound.cpp type-system issues.

## P3 — Build Round 2 (sandbox rebuild, 2026-11-14)

Rootless Debian sandbox: user-local mingw cross toolchain
(`scripts/setup-mingw.sh`, GCC 14 posix), cmake via pip, wrapper `scripts/d2env.sh`.
Configure passed — all 25 modules match project.json.
- **Fix 1**: strict Game module gate failed → BUILD BLOCKER #1.
- **Fix 2**: batch typedef drift between fork main and the ported tree
  (Fog/Debug.h `_EXCEPTION_DISPOSITION`, …), all resolved against origin/main /
  upstream 810e5018.
Two rebuilds: **328 → 94** deduped errors (~77%, first failure D2Game).
ItemMods.cpp (245 errors — `D2PropertyFuncEntryStrc::pfnHandler` signature),
Windowed.cpp (31), Direct3D.cpp (21), DirectDraw.cpp (13), glBlocks.cpp (8),
d3dFloorCache.cpp (8) cleared.

## P4 — Build Rounds 3–4 (compile cleanup)

Hotspots finished: Glide.cpp (54 errors), D2Game unit/quest cast sites, D2Launch/
MainMenus.cpp (91), D2Win/D2MCPClient, D2Client + D2Sound + D2Common message/
sound/UI rounds (~120 errors). Seven fix batches logged in port-plan.md §R4.

## P5 — Link round (Round 4 addendum): the port's biggest defect

Linking exposed whole TUs stubbed/emptied by the port. Restored verbatim from
origin/main: `ui.cpp` (282-line stub → 10,691 lines), `D2Multi.cpp`,
every `globals/*.cpp` untyped-data namespace block (21 modules), nine dropped
data/globals headers, `d2_boot.cpp`/`d2_hangwatch.cpp`, VP blocks (Fog
portable-save, D2WinFont shift-compare + labels, GuardStack), `SSComp.cpp`.
Deleted the port-created 4,008-line `D2ClientRoot.cpp` near-duplicate (118 ODR
collisions) — later re-added, see P7. Gates: documented EXCEPTIONS for
crashy/Game/D2Lang/D2Net. **build66: 0 compile errors, 0 undefineds,
0 multiple definitions** — link in progress.

## P6 — Fork reconciliation + link rounds

Module gates relaxed per CI decision (build.yml). Link-fix round: undefineds
**31 → 13** (fork month-strings block grafted into SFile.cpp; ITEMS_SearchItemCodeTable
decl strategy; per-fork Huffman-table casts; Storm list-head spelling).

## P7 — LINK SUCCESS (build133)

**d2.exe linked: 0 errors, 0 undefined references** (~333 MB debug binary).
Final 13: SkillsBar namespace wrapping (bare namespace → D2Client::SKILLS::),
INV_ResetItemDescFlags calling-convention, three naive param retypes replaced by
whole bodies from `asm_matching/msvc8/translated` (Message.cpp, Direct3D.cpp,
SkillsBar.cpp), WARDEN_DecryptBuffer fastcall retype, TEXT_InitializeTextSystem
relative-namespace fix, SOUND_GetEAXEnabledFlag def in the compiled Sound.cpp.
Lesson recorded: check the enclosing namespace before appending defs; the
translated tree is the source of truth for port bodies.

## P8 — CI duplicate-definition fix (overlay-aware)

CI (which extracts each archive over the previous state) failed with 123
duplicate symbols: the locally deleted `D2ClientRoot.cpp` had left a stale
`.obj` that masked missing defs, and archive-7-era defs added to D2Client.cpp
coexisted with Root.cpp's originals on a clean CI tree. Fixes: restored
D2ClientRoot.cpp (tree = pure superset), removed 107 duplicate namespace defs
from D2Client.cpp (`scripts/dedup_d2client.py`, nm-intersect), repaired the
dedup collateral (4 eaten forwarders), corrected WARDEN_DecryptBuffer to
__thiscall, D2Client.cpp includes D2ClientRoot.h. Clean CI-equivalent rebuild:
**0 errors, 0 duplicates, 0 undefineds**. Lesson: only ship complete supersets;
never trust a possibly-stale local build.

## P9 — First runtime field report + triage (archive #9, 2026-11-15)

Field: boots, title, char select, world at 800x600, VP Options toggles work.
Three issues triaged:
1. **Wide-res broken above 800x600 — FIXED**: `CLIENT_SetScreenResolution`
   switch had only retail cases 0/2; wide modes 3–6 fell through, leaving
   ScreenSizeX/Y at 800x600 (UI offset 0 → not centered; cull/room-walk budget
   800x600 → black corners). Cases 3=1344x700, 4=1280x720, 5=1366x768,
   6=1920x1080 added; compiles.
2. **Shared stash tab inert — FIXED**: client chain complete, but the server
   dispatch (PlayerTrade.cpp button-0x7D → VanillaPlusStash) was dropped in the
   overlay era; block recovered verbatim from origin/main + include; compiles.
3. **Tooltip background missing — OPEN**: full chain verified intact statically
   (75% black rect, palette color, all renderer tables wired); field repro
   questions recorded in handoff.md §3.
Docs repurposed this session: handoff.md (port handoff) and this file
(port history); fork originals preserved on origin/main.
