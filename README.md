<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/bd850c32-2a98-477f-bda9-a22e41d18bbd" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/999da238-d2f1-4c83-9a01-910daeea65a2" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/fb46e254-b1ce-44b7-a59c-0736af6ce05d" />

# D2VP Vanilla+QoL

<a href="https://madeopenly.com/?t=325ab9ffa5494159843c9f9999b41dc3" title="Made Openly: Mostly AI">
  <img src="https://madeopenly.com/b/325ab9ffa5494159843c9f9999b41dc3.svg?l=mostly-ai&s=dark&z=small&v=robot&m=robot"
       width="88" height="31" alt="Made Openly: Mostly AI">
</a>

[![Release](https://img.shields.io/badge/release-0.0.8--R137-8f2b2b)](https://github.com/VTSTech/D2VP-QoL/releases/latest)
[![Gates](https://img.shields.io/badge/vanilla%2B%20gates-524%2F524-2ea043)](tools/check-vanilla-plus.sh)
[![Fork of](https://img.shields.io/badge/fork%20of-jaenster%2Fredivivus-2b5797)](https://github.com/jaenster/redivivus)
[![Discord](https://img.shields.io/badge/Discord-join%20the%20chat-5865F2?logo=discord&logoColor=white)](https://discord.gg/xQXpH5Nj5)

Diablo II: Lord of Destruction 1.14d, recovered as C++ that builds and runs — with
**Vanilla+** on top: a fix-and-QoL layer maintained by VTSTech directly in the source.
Crash fixes, widescreen UI support, opt-in quality-of-life features and a settings page —
nothing silently changes gameplay. The client ships as **`d2vp.exe`**; rendering through
Vulkan comes from the separate [Glide3xVK](https://github.com/VTSTech/Glide3xVK) project
(`glide3x.dll` with a libSlang shader chain).

## Native Linux Port (branch `linux`)

A native Linux build **`d2linux`** is available on the `linux` branch — an SDL3 host that
runs the recovered game logic directly on Linux (i386, byte-identical to the PE build).
See [`docs/linux-port.md`](docs/linux-port.md) for the build guide and milestone status.

The tree is organised by the module names the game itself uses (`D2Client`, `D2Common`,
`D2Game`, `D2Win`, `D2gfx`, `Fog`, ...). Names, types and structure are reconstructions:
some functions still read like a decompiler wrote them, because one did.
[CONTRIBUTING.md](CONTRIBUTING.md) lists which paths are generated and which are
hand-written.

## Scope