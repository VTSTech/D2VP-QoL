<img width="800" height="600" alt="image" src="https://github.com/user-attachments/assets/804a905a-7050-4870-b6a9-e87ea8c60577" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/bd850c32-2a98-477f-bda9-a22e41d18bbd" />
<img width="1536" height="863" alt="image" src="https://github.com/user-attachments/assets/c1302bd8-0544-4810-9b23-b53554312a32" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/999da238-d2f1-4c83-9a01-910daeea65a2" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/3f776f89-2545-41bf-8927-5b7513241d81" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/416c240a-8f74-48d0-8749-285b3af1a315" />

# D2VP Vanilla+QoL

<a href="https://madeopenly.com/?t=325ab9ffa5494159843c9f9999b41dc3" title="Made Openly: Mostly AI">
  <img src="https://madeopenly.com/b/325ab9ffa5494159843c9f9999b41dc3.svg?l=mostly-ai&s=dark&z=small&v=robot&m=robot"
       width="88" height="31" alt="Made Openly: Mostly AI">
</a>

[![Release](https://img.shields.io/badge/release-0.0.8--R168-8f2b2b)](https://github.com/VTSTech/D2VP-QoL/releases/latest)
[![Build](https://github.com/VTSTech/D2VP-QoL/actions/workflows/build.yml/badge.svg)](https://github.com/VTSTech/D2VP-QoL/actions/workflows/build.yml)
[![Discord](https://img.shields.io/badge/Discord-join%20the%20chat-5865F2?logo=discord&logoColor=white)](https://discord.gg/xQXpH5Nj5)

Diablo II: Lord of Destruction 1.14d with **D2VP+QoL** on top: a crash fix and Quality of Life layer maintained by VTSTech directly in the source. Features true in game rendered widescreen (not stretched), 60FPS Unlock, portable saves, auto-gold pickup, iLvl tooltips, shift compare, ctrl+click xfer and many more opt-in quality-of-life improvements, all toggled via a settings page. No balance changes, no save-format changes. 100% Compatible with 1.14d Retail saves.

**Client:** `d2vp.exe` | **Renderer:** Separate [Glide3xVK](https://github.com/VTSTech/Glide3xVK) project

## Features (opt-in, default OFF)

| Feature | `d2vp.ini` key | What it does |
| --- | --- | --- |
| Show Items Toggle | `[features] alt_toggle` | Alt press flips ground-item labels on/off (click to pick up) |
| Portable saves | `[features] portable_save` | Saves read/write `<game folder>\save` (registry untouched) |
| Auto Gold Pickup | `[features] auto_gold` | Gold collected automatically when walking over piles |
| Persistent Globe Text | `[features] globe_persistent` | Life/Mana readout stays visible without hovering |
| Item Level Tooltips | `[features] item_ilvl` | Appends ` (N)` to item name on tooltips |
| Shared Stash | `[features] shared_stash` | 100 shared stash pages (100+ retail-shaped pages) |
| No Panel World Shift | `[features] no_panel_shift` | World stays in place when panels open |
| Mana Burn Fix | `[features] mana_burn_fix` | Fixes retail's Mana Burn bug to match D2R post-2.4 |
| Shift Item Compare | `[features] shift_compare` | Shift+hover shows equipped item comparison |
| Ctrl Click Transfer | `[features] ctrl_click_xfer` | Ctrl+Click moves items inventory↔stash (D2R parity) |
| Extended Stats | `[features] extended_stats` | Character screen gains Extended tab with hidden stats |
| Unlock FPS | `[features] unlock_fps` | ~60 FPS present while game stays 25 Hz (default On) |
| FPS Interpolation | `[features] unlock_fps_interp` | World glides between game ticks (default On) |
| Automap Clock | `[features] automap_clock` | Shows session time and system clock on automap |
| Transparent HUD | `[features] transparent_hud` | Bottom bar background, run/menu buttons hidden |
| Camera Zoom | `[features] camera_zoom` | Press **Z** to zoom world (1.0→1.25→1.5→1.0) |
| Palette 37 Tints | `[features] palette_37` | Full tint table: 8 tint rows retail never rendered start appearing on champions/uniques |

## Install

1. Download latest release from [releases page](https://github.com/VTSTech/D2VP-QoL/releases)
2. Extract into your Diablo II: LoD 1.14d folder (next to `Game.exe`)
3. Run `d2vp.exe -w` (Vanilla+ client)
4. Optional: `d2vp.exe -nointro` skips intro videos

**Upgrading:** Extract over previous install (saves and `d2vp.ini` carry over)

## In-Game Settings

Press **Esc → Options → "Vanilla+ Options"** to toggle features in-game. All settings hot-re-read every ~2 seconds.

## Current Status

- **Version:** Vanilla+0.0.8-R168
- **Renderer:** D3D/Glide via [Glide3xVK](https://github.com/VTSTech/Glide3xVK)
- **Scope:** Single-player only, Battle.net not supported
- **Compatibility:** Full backward compatibility, `.d2s` files untouched

## Quick Start

- Default config auto-created if missing
- All features OFF by default (except `unlock_fps_interp`)
- No game balance changes
- Field-verified crash fixes and features

## Build

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-i686.cmake
cmake --build build  # -> build/d2vp.exe
```

See [docs/building.md](docs/building.md) for detailed build instructions.

## Links

- [CHANGELOG.md](CHANGELOG.md) - Release history
- [FEATURES.md](FEATURES.md) - Feature details
- [CONTRIBUTING.md](CONTRIBUTING.md) - Development guide
- [LICENSE](LICENSE) - MIT License

---

*D2VP Vanilla+QoL is a fork of [Redivivus](https://github.com/jaenster/redivivus) with Vanilla+ enhancements by VTSTech.*
