# Round summary — R139 (rev 153, 2026-11-09): the on-screen build tag

Owner: **"For R139 can we change the 'V 1.14D' Title Screen/Automap Text to be 'D2VP-Rxx'"**.
Code round: the title screen and the in-game automap overlay no longer show the retail
version line ("v 1.14d") — both show **`D2VP-R139`**. Self-ID bumps to
**`Vanilla+0.0.8-R139`** (line 1 of `d2boot.log`). First round after the rev-152 cleanup.

## What changed (4 code files)

- `Redivivus/d2_boot.h` — **NEW defines** (one line to bump each round):
  `D2VP_ROUND "139"` + `D2VP_BUILD_TAG "D2VP-R" D2VP_ROUND`. The header every
  boot-marking TU already includes.
- `Source/D2Launch/MainMenus.cpp` — `UIMENU_MainMenu`'s version text box (form 0x115)
  prints `D2VP_BUILD_TAG` instead of the `sprintf "v %d.%d%c"` line. The retail
  internal-build append (`D2IniData_launcher->bBUILD` → `" 71"`) is retired with the old
  text — the tag IS the build identity now. The struct field stays at 0x223 (zero layout
  change; only the read site is gone).
- `Source/D2Client/UI/Automap.cpp` — `AUTOMAP_DrawVersion` prints `D2VP_BUILD_TAG` (same
  swap). The TU gains the `Redivivus/d2_boot.h` include — the module-gate-safe pattern
  five D2Client TUs already carry (`ClientModeInGame`, `Game.cpp`, `Msg.cpp`,
  `Roster.cpp`, `SCmd.cpp`).
- `Source/Game/Main.cpp` — the boot self-ID composes `"Vanilla+0.0.8-" D2VP_ROUND` →
  `Vanilla+0.0.8-R139`, so d2boot.log line 1 stays in lockstep with the on-screen tag.

A future round bump is ONE line: `D2VP_ROUND` in `Redivivus/d2_boot.h`.

Marks: `// VP-BEGIN/END [build-tag R139]` at both draw sites. Deliberately untouched:
the crash-report milestone string (`ErrorManager.cpp`, "D2 1.14d") — report metadata,
not screen text.

## Docs

- `CHANGELOG.md` — R139 entry (top of the 0.0.8 section).
- `README.md` — release badge + Current build → R139; the since-R128 list gains the tag.
- `SMOKE-CHECKLIST.md` — build line + item 0 → R139; new **§Q** (items 55–56: title +
  automap show the tag). The stale `tools/gen-smoke-issue-template.py` reference is
  updated (the script was removed at rev 152).
- `.github/ISSUE_TEMPLATE/smoke-test.md` — §Q appended by hand (generator removed at
  rev 152; both files note the hand-sync).
- `handoff.md` → **rev 153**; the rev-152 paragraph offloaded verbatim to **H58**;
  **H59** = the R139 record; §1 self-ID line + §8 H-index updated.
- `HISTORY.md` — **H58** (the rev-152 paragraph, byte-identical offload) + **H59**.

## Suggested commit split (3)

1. `Vanilla+0.0.8-R139` — the four code files (build tag + self-ID bump).
2. `docs: rev 153 - R139 build tag; rev-152 offloaded to H58` — handoff.md, HISTORY.md,
   ROUND-SUMMARY.md.
3. `docs: R139 in CHANGELOG/README/SMOKE + template §Q` — the four docs.

## Verification

- Sandbox (no cross toolchain — the compile gate of record is your Win11 build):
  repo-wide format-string sweep — **zero** `"v %d.%d%c"` sites remain;
  `D2VP_BUILD_TAG` consumed at exactly the two draw sites; `D2VP_ROUND` consumed by the
  self-ID compose; `nVersionLen` fully removed; no struct/layout edits; the edits are
  string-literal swaps + one adjacent-literal concat (`"Vanilla+0.0.8-" D2VP_ROUND`).
- Field: **§Q item 55** — the main-menu version text reads `D2VP-R139`;
  **item 56** — the automap overlay's top-right version line reads `D2VP-R139`
  (ride-along: level name / difficulty / password / server IP lines unchanged;
  tag fully on-screen at 640x480).
- d2boot.log line 1 must say `Vanilla+0.0.8-R139`.

## Still owed (unchanged from rev 152)

- §M2 `gamble:` log-chain discriminators, §M3 one purchase, §M4 the same-session
  re-open (the §M1 click is green on the salvage build).
- The explicit 16-bit gossip log check (the `msgType0=` mark + the 0x31 echo).
- SMOKE ride-alongs on any future build; §P item 54's 1366x768 `-w` select doubles as
  the R116 eyeball.
- §7 triages: #25 (track R, shrine XP buff), #18 (track Q, attribute-point crash).
