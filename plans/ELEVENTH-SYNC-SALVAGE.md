# Eleventh-sync salvage review (read-only, no merging)

> **STATUS UPDATE (rev 146, 2026-11-08 — salvage round 1 SHIPPED):** owner go
> "Salvage what you can, whatever is least likely to break anything." All four
> TAKE items landed in R135 as independent drops: `228dfc62` (verbatim),
> `d6746a33` (with the required VP clamp at the diagnostics mark; R128
> static+accessor retired with dated notes; gate rows re-pinned),
> `83b045a4` (Text.{h,cpp} verbatim; ui.cpp's quest-gossip marks threaded),
> `c16c2a68` (all 11 real files clean; cross-sweep clean). `92f67c4f` stays
> OPTIONAL/deferred. The scroll-box recon note below got a decisive addition:
> the fork's scroll chain matches upstream's byte-match state, and
> `a633e56a`'s `TEXT_UpdateScrollingText` reorder is the new prime suspect —
> see handoff rev 146. Suite 520/520; self-ID `Vanilla+0.0.8-R135`.

Owner decision of record (2026-11-08): **upstream syncs are PAUSED.** The eleventh
sync (merge `17d8a51b`, R135) cost a full day and never reached a clean boot; when a
build finally ran, textures were corrupted. The whole round was rolled back to
`8067c4a` (R134) in `86cd10cd` — the tree there is byte-identical to R134 and is the
working base. The fork continues by improving the R134 source directly. Upstream
material, when wanted, gets hand-ported one commit at a time under the house rules
(VP marks, gate rows re-pinned never deleted, self-ID bump per code round, cross-sweep
before handover, owner runs MSYS2 + smoke).

This document is the ranked verdict on everything the paused window contains. Scope
checked 2026-11-08: the eleventh sync carried upstream `30384f1a..a633e56a` (10
commits, 1498 files, Ghidra DB 1970 → 2014, +52,199/−56,707), and `origin/jaenster-main`
has moved **only CI commits** since `a633e56a` — so this range is the entire window of
"what upstream has that we don't". Nothing newer is hiding behind it.

One bookkeeping note: `5841e76c` inside that range is the fork's own README commit —
jaenster's main had already absorbed the fork's main before the window opens, so the
window is nine upstream-authored commits.

---

## The ranked take-list

### 1. `d6746a33` — gamble table is one 0x198-byte table again — TAKE (best value in the window)

3 files, +11/−13. This is the **retail-exact twin of the fork's own R128 gamble fix**
(issue #32, track T, H44) with a better ending state.

- **The defect, upstream's diagnosis:** `D2GambleTableStrc` was typed as only its
  first 8 bytes (`nSize` + `void* pTable`). The loader (`TXT_AllocTxt_gamble`) wrote
  the per-level limits into a separate static array (`gnItemTblsCacheSize[100]`) while
  `GAMBLE_GenerateGambleInventory` read them past the end of the 8-byte table — data
  adjacency that only worked in the original binary's `.data` layout, exactly the class
  the fork's R128 record describes. Upstream found it independently (issue #104).
- **The fix:** `anLevelLimit[100]` moves back inside the struct (8 + 400 = 0x198 bytes,
  one table in the original), `pTable` becomes `int*`, loader and generator use the
  same array.
- **What R134 has instead:** `static int gnItemTblsCacheSize[100]` at file scope
  (ItemTbls.cpp:123), the bounds-checked accessor `GetGambleLevelCache(int32_t)`
  (decl ItemTbls.h:219 outside the module gate, def ItemTbls.cpp:1862), and the
  consumer at SUNITNPC.cpp:2200 (VP-marked `[gamble-cache-fix R128]`) plus the
  diagnostics mark at SUNITNPC.cpp:2177. Four gate rows pin it
  (`tools/check-vanilla-plus.sh:1096-1099`).
- **Port plan:** extend the struct in `ItemTbls.h`; loader writes
  `anLevelLimit[...]` in place; both SUNITNPC sites re-express onto
  `pGambleTable->anLevelLimit[...]`; retire the accessor and the static with dated
  notes; re-pin the four R128 gate rows onto the new spellings (rows re-pinned, never
  deleted — house rule).
- **Keep one VP guard:** the roll site is in-bounds by construction (the reused
  `pPlayerUnit` slot is clamped 5..99 immediately above). The diagnostics mark reads
  `nPlayerLevel` straight from `STATSEX_GetCurrentStat`, which is not clamped — keep a
  VP-marked clamp (0..99) there rather than reading `anLevelLimit` unclamped.
- **Ordering:** port before `c16c2a68` (both touch SUNITNPC.cpp).
- **Smoke owed:** the standing Gheed gamble click pass (SMOKE item, track T).

### 2. `228dfc62` — SREG_InternalSaveEntry takes its flags as a dword — TAKE (easiest)

1 file (3rdparty/Storm/Source/SReg.cpp), 8/8 lines. The registry-write worker receives
the caller's flags in EBX and pushes the whole register; typed as `uint8_t`, every
caller truncated. Zero behavior change for the flags actually tested (0x10/0x4/0x8 all
fit a byte; the one byte-typed callee `SRegGetBaseKey` keeps its cast). Pure asm
accuracy, self-contained, no fork marks in the file. Port any time; a good first
hand-port to exercise the process on something trivial.

### 3. `83b045a4` — TEXT/OVERHEAD message getters return a 16-bit id — TAKE, with care

10 files, +44/−60, but only **4 real files**: `Source/D2Common/Text.{h,cpp}` (the
getters re-typed `uint32_t` → `uint16_t`) and the two client call-site TUs
`Source/D2Client/UI/npcmenu.cpp`, `Source/D2Client/UI/ui.cpp`. Skip the version
chores (`.recomp.json`, README, `ghidra_version.cpp` — the fork pins Ghidra 1970 by
the rollback) and the `.cpp.map` sidecars.

- R134 still has the pre-fix signatures (`Text.h:48-49`, `uint32_t` returns).
- **Both client TUs are fork-marked files** — ui.cpp carries the dismiss table and
  boot-log marks, npcmenu.cpp the R60 roll-fail marks. Thread the VP marks through the
  re-typed call sites; the 16-bit form actually *simplifies* several
  `(short)/(uint16_t)` round-trips the fork's marks sit next to.
- **Bonus while in there:** this is exactly the NPC-menu overhead/scroll machinery
  behind the known issue "when an NPC has quest news, the scrolling text box does not
  appear" (README). The retyping does not fix it (bit patterns identical), but the
  recon pass through `NPCMENU_FindNextUnhandledMessage` and the scroll queue is free
  evidence-gathering for that bug — note findings in the round's handoff rev.

### 4. `c16c2a68` — seed, affix, NPC and inactive-item pointer typing — TAKE (batched)

15 files, +90/−97; 13 real files (skip `.map`). Replaces int-cast round-trips with
typed pointers across `D2Client/UNIT/Item.cpp` (the big hunk, 104 lines),
`D2Common/Seed.{h,cpp}`, `D2Game` monster AI / skills / unit-damage / inactive-unit
files and SUNITNPC. Behavior-neutral typing contracts; port after `d6746a33` for the
SUNITNPC overlap. Cross-sweep mandatory (fork edits in upstream files).

### 5. `92f67c4f` — monster, NPC, party-flag and wide-string helper contracts — OPTIONAL

21 files, +149/−146; 19 real files. Same class as #4 but with the **widest
fork-conflict surface of the small commits**: `D2WinFont.{h,cpp}` carries the
shift-compare second channel (R125/R126 gates pin counts there), `ui.cpp` carries the
dismiss table, `Monster.cpp/h` is fork-touched from the R50/R51 polarity fix, and the
D2Client.h/globals.h declaration moves interact with the two-copies header rules.
Value is accuracy, not behavior. Take it as its own round when convenient, not first.

### 6. `a633e56a` — cherry-pick by area, do not merge

57 files, +1,500/−274, mixed bag. Useful parts, each independently portable:

- **SPELLSEL charged-skill helpers return int** — small typing fix.
- **"Branch and statement order follows the original in 16 more functions"** — the
  list names the **text scroll box** and **shrines**. These are byte-match accuracy
  rounds, not claimed bug fixes, but they are the byte-accurate reference for two open
  tracks: #25 (Refilling Shrine XP buff) and the quest-news scroll box. When triaging
  either, diff the fork's version of those functions against `origin/jaenster-main`
  (read-only reference; it is checked out in the working clone) instead of re-deriving
  order from the decompiler.
- **"The GCC build installs the runtime patches at startup"** and **"startup stops
  with a clear message when the 1.14d level data is missing"** — developer-QoL items
  relevant to a mingw-built fork; portable on their own if wanted.

### 7. `272e075e` + the x64 half of `a633e56a` — PARK

248 files ±~1.9k lines (53.0% matched) plus the x64 count tooling (53.1%, 7,768 of
14,626 functions, 2,390 pointer-width casts left across 463 files under VC8 x64).
Strategic for a hypothetical x64 build; the fork targets i686 mingw. Revisit only if
an x64 campaign becomes a goal — by then most of it will need re-doing anyway.

---

## Do not port

- **`b62994ba` (include-only rewrite, 558 files, −32,988 lines)** and **`7b702558`
  (one public header per module, 1423 files, +48,770/−22,278)** — restructure for
  upstream's tooling. The eleventh sync already proved the cost: every transitive
  include path broke (15 TUs needed `d2_boot.h` re-plants), the public-header split
  spawned the two-copies header rules and three of the five fallout-fix commits exist
  only to reconcile the fork with these two commits. Porting them means re-buying that
  entire cost for zero gameplay value.
- **The sync's Glide texture-cache re-expression — the field-failure suspect.** The
  merge re-expressed the fork's fix-2 stale-index validation on upstream's typed
  `GlTextureCacheNodeStrc` shell (new `+8/+0xc` layout offsets) and re-planted fix-3's
  guard on upstream's `nCacheBucket`/`(char*)` spellings. Corrupted textures on the
  R135 build point here first (the texture-cache guards are exactly what was
  rewritten). **R134's fix-2/fix-3 are field-proven; do not touch them on upstream's
  authority.** If a post-mortem is ever wanted:
  `git diff 8067c4a 17d8a51b -- Source/D2Glide/Renderer/Glide.cpp` is the place to
  look (the lost chain is still reachable: tip `b3104e6f`).

## Process for every hand-port (the house rules, restated)

1. One salvage item = one code round = one self-ID bump (`Game/Main.cpp`) + gate row
   re-pin + SMOKE items relevant to the area.
2. VP-BEGIN/VP-END marks on every edit inside an upstream file; fork-owned TUs for
   anything new.
3. Cross-sweep with the exact CI flags before handing over; owner runs the MSYS2
   build + link + in-game smoke (the compile gate of record).
4. Gate rows re-pinned, never deleted; supersessions get dated notes.
5. Docs: handoff rev per round (16 KB budget, trim to HISTORY per the pattern);
   CHANGELOG entry only when a release ships.
