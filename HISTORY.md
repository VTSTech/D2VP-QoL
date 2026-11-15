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

## P10 — Field round 2 (archive #9 results) + camera-zoom fix (archive #11, 2026-11-15)

Field: **wide resolutions FIXED** (all stops render, centered, no black corners),
**shared stash FIXED**, shift compare + Unlock FPS/interp survived. Two open:
tooltip backgrounds still missing; Z camera zoom scales characters/NPCs but not
the camera view.

**Camera zoom — FIXED (root cause)**: the fork's `Source/D2Client/GAME/Draw.cpp`
carries the R155 zoom sites — the top-down floor/roof light-grid scaling
( :500/:558, "the grid cells ARE the ground-tile vertices") — plus the R118
wide-view floor walk and R121 bounds, ALL missing from the port's trimmed copy
(1389 vs 1724 lines, zero `GetCameraZoomFactor` consumers in D2Client). That is
exactly the symptom: sprites scale via the renderer-side quads, ground tiles
stay 1:1. Recovered the fork file wholesale (our copy had no unique port fixes;
only the umbrella-include adaptation), added the missing
`WALL2_CollectRoomsBeyondAdjacency` declaration to Wall2.h (fork :79), fixed the
one compile error, full build green. Side benefit: the wide-view FLOOR walk is
restored too (the fixed wide-res so far ran on the Wall2-side collector only).

**Tooltip background — still open**: the entire chain is verified correct
statically (75% black rect via DrawSolidRectEx(0, DRAW_LESSTRANSPARENT=2), palette
color, all renderer vtables wired). Next-round plan: field test with
d2boot.log renderer mode + a screenshot, and check whether Draw_UnitNameUnderCursor
is reached at all in-game (the VP diag era is retired; add a temporary
D2BOOT_Mark probe if needed).

## P11 — Win11 local build script fix (archive #12, 2026-11-15)

The owner's `tools/build-local-win11.cmd` run (MSYS2 mingw32, **GCC 16.2** — newer
than the sandbox's Debian cross GCC 14) failed at the first TU:
`3rdparty/Storm/Source/SFile.h:26 conflicting declaration 'typedef int
_EXCEPTION_DISPOSITION'` — the newer mingw-w64 excpt.h (pulled in by windows.h)
typedefs `_EXCEPTION_DISPOSITION` as the SEH **enum**, while the port's 20
per-file `typedef int _EXCEPTION_DISPOSITION;` fallbacks assume the OLD
mingw-w64 headers (Debian snapshot: excpt.h defines the enumerators as MACROS —
`#define ExceptionContinueSearch 1` — and no such type at all).

Discriminator chosen without depending on header guard names:
`defined(ExceptionContinueSearch)` is TRUE exactly where the macro flavor is in
use (old headers) and FALSE where the real enum exists (new headers), so each
per-file fallback is now wrapped in `#if defined(ExceptionContinueSearch)`.
Verified by preprocessor probe on the sandbox toolchain and a full clean-ish
rebuild (`Built target d2`). The .cmd/.ps1 script itself needed no change — it
mirrors CI's configure and the failure was header-level. Note: GCC 16 vs 14
header drift may surface MORE conflicts after this one — next errors go to the
next round one at a time.

## P12 — Field round 3 (zoom verified) + alt-toggle fix (archive #13, 2026-11-15)

Field: **Z camera zoom VERIFIED FIXED** (view zooms with the world). Open:
tooltip backgrounds still transparent (chain already statically verified; field
diag still planned — renderer mode from d2boot.log, reach-probe on
Draw_UnitNameUnderCursor); **Alt item toggle dead** — fixed this round.

**Alt-toggle — FIXED (root cause)**: the classic overlay-era loss again. The
feature's five block homes: `VanillaPlusCore.cpp` config + latch helpers
(present), `ShowItems.cpp` stale-clear blocks incl. the R58 real-geometry
HitTestScreen (present), `UI/Draw.cpp` latch reset on wholesale UI reset
(present, placement fork-identical), and **`D2Client.cpp` CMD handlers (MISSING
ENTIRELY)**. In the port the two functions live in `D2ClientRoot.cpp` (the P8
canonical def home) with vanilla bodies only: `CMD_SetHoldAlt` never flipped the
latch, `CMD_ReleaseHoldAltAndClearSelection` never converged the flag — toggle
mode could never engage even with `alt_toggle=1`. Both VP blocks recovered
verbatim from origin/main into D2ClientRoot.cpp + the VanillaPlusCore.h include.
Hold mode (`alt_toggle=0`) byte-for-byte vanilla by construction. Build green.

Field asks (SMOKE §C item 8 + R58 guard): `alt_toggle=1` → Alt down-up flips
labels, brush a label and click empty ground = walk, label/sprite click =
pickup, combat click = attack; `alt_toggle=0` = vanilla hold unchanged.
