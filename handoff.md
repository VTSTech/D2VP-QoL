# main branch — handoff (rev 145, 2026-11-08 — **the rollback round: upstream syncs PAUSED**). The eleventh upstream sync (merge `17d8a51b`, R135) failed field acceptance: a full day without a clean boot, and the build that finally ran had corrupted textures — prime suspect the sync's re-expression of the fork's Glide texture-cache guards (fix-2 onto upstream's `GlTextureCacheNodeStrc` typed shell with new +8/+0xc offsets, fix-3 re-planted on upstream's `nCacheBucket` spellings); R134's fix-2/fix-3 are field-proven and stay untouched. Owner rolled back to `8067c4a` (**R134**) in `86cd10cd` — verified byte-identical (`git diff 8067c4a HEAD` empty), `.recomp.json` Ghidra pin back to 1970. Decision of record: **syncs are PAUSED, not retired** — they may resume when a process exists that bounds the cost; until then the fork improves the R134 source directly and upstream material is hand-ported one commit at a time. The salvage review landed at **plans/ELEVENTH-SYNC-SALVAGE.md**: the paused window is exactly the nine upstream commits `30384f1a..a633e56a` (`origin/jaenster-main` has moved only CI commits since) — TAKE `d6746a33` (gamble table back to retail's one 0x198-byte layout: struct regains `int anLevelLimit[100]`, `pTable` typed `int*`; supersedes the R128 accessor workaround — upstream hit the same defect class independently as its issue #104; keep a VP clamp at the SUNITNPC.cpp:2177 diagnostics mark whose `nPlayerLevel` is unclamped, the roll site is clamped 5..99 by construction; re-pin the four R128 gate rows, rows never deleted), `228dfc62` (SREG_InternalSaveEntry flags dword — easiest, 8 lines, no fork marks in the file), `83b045a4` (TEXT/OVERHEAD getters return 16-bit — skip its version chores; npcmenu.cpp/ui.cpp are fork-marked; doubles as recon for the quest-news scroll-box known issue), `c16c2a68` (seed/affix/NPC/inactive-item pointer typing — after d6746a33, SUNITNPC overlap), optionally `92f67c4f` (widest fork-conflict surface: D2WinFont shiftcmp gate counts, ui.cpp dismiss table); consult-don't-merge `a633e56a`'s statement-order rounds (text scroll box, shrines) when triaging the scroll-box known issue or #25; PARK the x64 typing (`272e075e` + the x64 half of `a633e56a`); **DO NOT PORT** `b62994ba`/`7b702558` (the include rewrite and the public-header split — the two commits three of the five fallout fixes were spent reconciling). This round (docs-only, self-ID stays **Vanilla+0.0.8-R134**): the self-ID gate row re-pinned R130 → R134 — R131–R134 (owner's local rounds, no handoff records: **R131** the UNISYS/ui.cpp round; **R132** glide-perf f2+f4; **R133** glide-perf f1+f3; **R134** glide-perf f5) bumped the self-ID but missed the suite's re-pin, so the suite ran **518/519 red before this round**; merge-upstream.md carries the PAUSED banner (archive, not procedure); CONTRIBUTING/README drop the next-sync promises; the rev-142 paragraph offloaded verbatim to **H47** (numbering note: the rolled-back R135–R139 chain had already used H47/H48; the numbers re-use after the rollback — renumber if anything from `b3104e6f` is ever resurrected). **Owner owes: nothing for this round; the next code round is salvage item 1 (`d6746a33`) on the owner's go, self-ID Vanilla+0.0.8-R135.** PREVIOUS rounds, one line each (full records: CHANGELOG + **H41/H42/H44/H45/H46/H47**): **R130** the tenth upstream sync, merge `a022ea27` (**H47** — its PREVIOUS list carries R117–R129); earlier eras per the H-index in §8.

## 1. Branch topology (owner-corrected, 2026-10-31; repo renamed + wrapper spun out, 2026-11-03)

- **Repository**: **`D2VP-QoL`** (renamed from `D2LoD-libslang-vk` 2026-11-03; old URLs redirect). **All Glide DLL work happens in the wrapper's own repository: [`VTSTech/Glide3xVK`](https://github.com/VTSTech/Glide3xVK)**.
- **`main`** — the only integration branch. The `720p` suite (merge `fec99ff`) and the vanilla-plus line are absorbed. Build self-ID **`Vanilla+0.0.8-R134`** (`Source/Game/Main.cpp:1152`; the rollback base `8067c4a`). Gates **519/519** after the rev-145 re-pin (`tools/check-vanilla-plus.sh`). Work happens here.
- **`720p`** — retired, read-only archive (its handoff era: **H25**).
- **PR branches** (`pr1..pr5`, `jaenster-main`) — upstreamed already; **not integration targets; do not merge from them.** Read-only references for fix specs (e.g. `pr4/sound-except-intro` = upstream PR #10, the sound trio).
- Full pre-720p history: HISTORY.md H1–H24 (H3 = the first sound-defect series record; H17 = the supersession precedent; H21 = fix-chain ledger rows 1–27).

## 2. Field status (collapsed current-state table at rev 137 — the per-round rows live verbatim in H40/H42)

| Area | Status |
| --- | --- |
| Boot → intro → title → char select → in game; D3D fullscreen/windowed; Glide `-3dfx`; sound end to end; `-nointro` (R86) | **WORKING** — field-verified; the full SMOKE belt green twice (R83 [#24](https://github.com/VTSTech/D2VP-QoL/issues/24), R104 [#28](https://github.com/VTSTech/D2VP-QoL/issues/28)) |
| Resolution ladder: 640x480 / 800x600 / 1280x720 retail + **1344x700 / 1366x768 / 1920x1080** (R109–R115), hosted stretch, windowed `-w` (R116 fix) | **WORKING** — field-proven R109–R115 ("Great job. works" at each stop); the explicit 1366x768-windowed eyeball still owed (§7); **[#31](https://github.com/VTSTech/D2VP-QoL/issues/31) open** — 1080p D3D-windowed selection crashes (fullscreen fine), track V, untriaged |
| Wide-view rendering campaign (R117–R122: band fill, hop-4 walk, cull-rect re-fit) | **CLOSED** — proven on vpdiag evidence; the probes retired R127 (zero behavior change) |
| D3D fullscreen boot crash (R124) + shift-compare shield slot (R126) | **FIXED — FIELD-VERIFIED 2026-11-06** ("d3d fullscreen works again" / "Great job, works") |
| Gheed gamble click-crash ([issue #32](https://github.com/VTSTech/D2VP-QoL/issues/32), R128) | **FIXED — FIELD-VERIFIED 2026-11-06, owner-closed** (baseline data-adjacency defect, record **H44**; §M stays as the standing gamble check) |
| Vanilla+ features (nine, all opt-in + Options rows) | **SHIPPED + FIELD-PROVEN** (shared stash ten pages R82; `no_panel_shift` R95–R101; mana burn R106–R108; shift compare R125/R126) |
| Upstream sync | **PAUSED** (rev 145) at Ghidra **1970**; the eleventh (→2014) rolled back; hand-porting only per plans/ELEVENTH-SYNC-SALVAGE.md |

## 3–5. Closed defect tracks A (sound) / B (glide) + re-litigation ledger — ARCHIVED

Track A sound (field-verified rev42), track B glide (R46, issue #11) and the §5 ledger moved verbatim to **HISTORY.md H26** at rev 58. Both tracks FIELD-VERIFIED — no open work; section numbers stay reserved so §6–§9 cross-references hold. The glide known-issue step is **H26 §4.3 step 4**.

## 6. Standing watches (carry-overs, unchanged)

- `roster heal:` in d2boot.log → upstream roster corruptor alive (rev35/36 stack; context H25). A cap mark now means REAL corruption, not a benign shape (R60 re-scope, issue #19 audit); the rev34–36 immunization contained the 2026-10-31 hit. If it recurs, send the log (hangwatch + `d2exe.map` as usual).
- Track J follow-up (gossip-data recovery) — RESOLVED at the R88 merge (upstream recovered the retail tables verbatim); the R53/R54/R85 `quest:` marks stay as field diagnostics.
- `s2c dispatch guard:` / repeating `s2c pump: drained` → packet-path pathology (rev34 hardening; H25 §8).
- hangwatch dumps at 12 s stalls; keep `d2exe.map` beside d2vp.exe for symbolization.
- d2boot.log first mark must read the current self-ID string — pins the exe before any analysis.
- SetPan right-branch negation deviation (stereo position only; H3 watch item).

## 7. Field checklist — the still-owed items

- **Issue [#31](https://github.com/VTSTech/D2VP-QoL/issues/31)** (track V, NEW 2026-11-06): 1080p D3D-windowed crash on mode select (fullscreen fine) — own-code AV WRITE, 22 frames, eip image +0x3FFB97; untriaged. Report `d2crash-20261106-003402.txt`.
- **Issue [#29](https://github.com/VTSTech/D2VP-QoL/issues/29)** (track U, NEW 2026-11-06): crash right after killing The Countess — halt reports line=2538 addr 005E7C31, line=290 addr 00780870, then VEH 0x007E8378 (all below the 0x00B90000 image base — attribute the module first); untriaged. Report `d2crash-20261105-195033.txt`.
- **Issue [#25](https://github.com/VTSTech/D2VP-QoL/issues/25)** (track R): Refilling Shrine XP buff — suspected wrong-shrine-effect application; triage pending.
- **Issue [#18](https://github.com/VTSTech/D2VP-QoL/issues/18)** (track Q): intermittent attribute-point crash — never triaged; reproduce with `d2exe.map` beside the exe, collect `d2crash-*.txt`.
- **Owner closes landed 2026-11-06**: #26 (track S — field-proven R92–R101, cleared by the R104 full smoke pass) and #8 (track C — field-verified R91, record §9.1) are closed on GitHub.
- **R116 owed**: the explicit windowed (`-w`) eyeball at 1366x768 (frame straight, no shear at the window edges) — `-w` boots clean on the R123/R124 field builds; plus the formal six-stop switch-ladder walk (the 1080p stop is field-proven by use).
- **Cosmetic label follow-ups** (from #26): overhead icons, alt-labels, the border-3 guards.
- Closed tracks (records in their H-entries): C uniques R49 [#9], C skeletons R51 (§9.1), I belt hover R52 (H27), J Den turn-in R54 (H28), M save-re-enter R61 [#19] (H31), P Bloodraven R85 [#22] (H35/H36), T gamble R128 [#32] (H44).

## 8. Conventions (condensed; full historical texts in H8/H42)

- **Docs policy (owner, 2026-11-06)** — two lanes: **human** = README.md + CHANGELOG.md (concise; CHANGELOG = one short entry per release) + SMOKE-CHECKLIST.md (live checks only; closed-round history → HISTORY); **agent** = verbose-OK: handoff.md, HISTORY.md (unbounded), `plans/*` (all plans), FEATURES.md (the slate), docs/*. **handoff.md has a hard 16 KB budget, gate-enforced** (`tools/check-vanilla-plus.sh`): when it grows past the cap, offload the oldest-again material verbatim into the next HISTORY.md H-record (the trim-N pattern, H25–H43) and re-run the gate. CHANGELOG entries stay short — the engineering detail goes in the handoff rev line and, when it trims out, HISTORY.
- **Overlay workflow**: sandbox edits the tree → overlay **zip of changed files** (regular rounds; bundles only for upstream merges) → owner applies as own commits → llvm-mingw i686 Win11 build (MSYS2 MINGW32). The full build is the owner's gate.
- **VP-BEGIN/VP-END markers** on every fork edit in upstream files; fork-only TUs (`VanillaPlusCore.{h,cpp}` at `D2Client/UI/`) keep upstream files free of new code.
- Every source change updates the `tools/check-vanilla-plus.sh` gate table. (The owner-side fork-surface budget/allowlist check is RETIRED rev43.)
- Line endings: **merged main is LF-normalized** (rev41 finding) — patchers detect and preserve each file's ending, never mix.
- **Version self-ID** (`Game/Main.cpp:1078`) bumps every code round so d2boot.log pins the build; docs-only rounds do NOT bump it. The suite carries a single self-ID row, repinned every round; the build check itself is manual: d2boot.log line 1 (SMOKE-CHECKLIST item 0).
- **Sandbox**: llvm-mingw i686 cross-toolchain available since R90b — no apt; **the owner's MSYS2 build stays the compile gate of record.** Per-round changed-TU compile checks must pass the CMake forced-include set (`-include d2_platform.h -include support/d2_hand.h`).
- Diagnostic channels: `d2boot.log` (`[d2boot]` marks, permanent), hangwatch stall dumps, the game log (`DirectSound error:` lines), `glide3x.log` (wrapper side).
- Full history: H1–H24 (pre-720p), H25 (720p era), H26 (0.0.6 post-release era), H27/H28 (tracks I/J), H29–H31 (rev offloads 57–109), H32 (README offload), H33–H40 (trims #8–#16: the R84-era fixes through the R89–R101 arc), **H41 (CHANGELOG full text, pre-restructure), H42 (handoff pre-trim #17), H43 (SMOKE pre-restructure), H44 (issue #32 gamble), H45/H46 (revs 140/141), H47 (rev 142, the tenth sync — offloaded at rev 145)**.

## 9. Known-issue manifest (rev-139 status: FOUR open — tracks Q/R/U/V; closed rows live in their H-records)

| # | Owner symptom (verbatim) | Track |
| --- | --- | --- |
| 12 | "**Bug** 'Refilling Shrine' gives XP buff" ([issue #25](https://github.com/VTSTech/D2VP-QoL/issues/25) — owner, 2026-11-02) | R — suspected wrong-shrine-effect application (XP belongs to a different shrine type); triage pending |
| 13 | "**Crash** Intermittent crash when applying attribute points" ([issue #18](https://github.com/VTSTech/D2VP-QoL/issues/18) — owner, 2026-10-31) | Q — never triaged; intermittent — reproduce with `d2exe.map` beside the exe, collect `d2crash-*.txt` |
| 15 | "Crash right after killing The Countess" ([issue #29](https://github.com/VTSTech/D2VP-QoL/issues/29) — owner, 2026-11-06) | U — untriaged; halt reports line=2538 addr 005E7C31 / line=290 addr 00780870, then VEH 0x007E8378 — all below the 0x00B90000 image base, attribute the module first; report `d2crash-20261105-195033.txt` |
| 16 | "[1080p] D3D Windowed crashes upon selecting 1080p - Fullscreen is fine" ([issue #31](https://github.com/VTSTech/D2VP-QoL/issues/31) — owner, 2026-11-06) | V — untriaged; own-code AV WRITE, 22 frames, eip 0x00F8FB97 (image +0x3FFB97); report `d2crash-20261106-003402.txt` |

Open: **rows 12, 13, 15, 16** (issues #25, #18, #29, #31 — tracks R, Q, U, V). Rows 14 (#32, track T — FIXED R128, the §M field pass green, record **H44**), 11 (#26, track S) and 3 (#8, track C) lifted to history at the owner's 2026-11-06 closes.

### 9.1 Track C — minion skeletons gold — FIXED R51, FIELD-VERIFIED R91 ("Skeletons are finally white again") — issue #8

A baseline transcription defect (`44e3518`) with two inverted-polarity write sites — the roster pet-recolor `(uint32_t)!bHostile` (R50) and the spawn-time `CreateObject` `Utrans==0xFF` fallback (R51, the write that sticks for SP summons); both now write the raw hostility flag. Full record verbatim: **H31** (original H26 Revs 55/56). Lifts to history at the owner's close.
