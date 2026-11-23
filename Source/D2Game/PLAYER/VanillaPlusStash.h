// vanilla-plus shared stash: server-side container (track N, round 1).
//
// Fork-owned TU (picked up by CMake's GLOB_RECURSE) - do not move this code
// into upstream files; upstream files only ever call the entry points
// declared here from thin VP-marked sites (handoff.md §8).
//
// Owner constraints (SHARED-STASH-PLAN.md, 2026-11-01):
//   1. "the stash should have it's own save"  - the shared pool lives in its
//      own file, never inside the character .d2s.
//   2. "vanilla+ character saves need to be compatible with retail" - hard
//      invariant I1: a Vanilla+ save loads in retail, byte-for-byte. The
//      shared page therefore NEVER survives into a .d2s: the save-flush
//      entry point below parks the shared set into the .sss and restores
//      the personal set before MakeSaveFile runs.
//
// Design (round 1 - one page, retail-shaped; two shared pages since R75,
// ten since R82, a hundred since R146):
//   While the shared page is active, the player's INVPAGE_STASH (page 4)
//   inventory holds the SHARED set - materialized from the .sss file - and
//   the personal set waits in a memory snapshot. Every retail stash flow
//   (grid draw, 0x19 pick, 0x18 place, gold buttons) works on it unchanged.
//   Shared gold borrows the retail UNITSTAT_goldbank stat while the page is
//   active (retail deposit/withdraw buttons + display work as-is); the
//   swap/restore happens only at the toggle and the save flush. Toggling
//   back (or saving) serializes the shared set back into the .sss using the
//   retail per-item encoder, so an item can move file -> .d2s -> file
//   forever without a conversion step.
//
// Scope guard (R204 - realm support): the feature engages when
// [features] shared_stash=1 in d2vp.ini, for EVERY game type - single-
// player (local server), Battle.net closed realm, Open Battle.net, LAN.
// Pools are PER ACCOUNT (R204, owner: "each player to have their own shared
// stash, not shared with other players, just shared for that one account"):
// the pool key is the player's account name, the file is
// "_d2vp_shared_<account>.sss" on the server (single-player keeps the
// legacy accountless "_d2vp_shared.sss"), and each character materializes
// its own account's page into its own page 4 with its own personal
// snapshot. Different accounts never touch each other's pools, so any
// number of players can use the shared stash at the same time - no
// exclusivity needed. Deployment note: on Battle.net realms this module
// runs on the realm host, so the D2VP-D2GS server build must include it
// (a vanilla d2gs silently ignores button 0x7D; harmless, no toggle).

#pragma once

// [R220] Self-sufficient: the standalone d2vpstash.dll target compiles this TU
// without the tree-wide forced includes, so the header must not rely on them.
#include <cstdint>
#include <cstddef>

struct SGAMEDATA;
struct D2UnitStrc;

// [vanilla-plus shared-stash] The default button id the client sends via
// C->S 0x4F (Click Button) to toggle the shared page. Single-player runs on
// this fork's own D2Game, so an out-of-retail id is fine (retail ClickButton
// routes unknown ids to the trade-path rejection; our VP site intercepts
// first). On REALMS d2server.dll's cheat validator drops every button id
// outside its retail whitelist BEFORE D2Game sees the packet, so there the
// id must be a whitelisted one - [features] shared_stash_button overrides
// this default on both ends (R205; see VanillaPlusCore.cpp for the client
// half and the probe notes in the D2VP-QoL findings doc).
constexpr uint16_t VPSHARED_BUTTON_TOGGLE = 0x7D;

// [R205] Default pad-word magic that marks a stash toggle. The page rides the
// packet's last u16; the pad u16 in front of it carries this value, so the
// fork can tell a stash toggle from a genuine retail click of the same
// button id (d2server.dll does not inspect the pad word). Both ends read
// [features] shared_stash_pad_magic; 0 = R204 shape (no magic).
constexpr uint16_t VPSHARED_PAD_MAGIC_DEFAULT = 0;

// TRUE when the VP site in PlayerTrade::ClickButton must divert this button
// to VPSHARED_OnClickButton. Requires: the configured toggle button id, the
// configured pad magic in dwParam's second word, and the feature ini gate on
// (any game type since R204). A genuine retail click of the same id carries
// no magic and passes through to the retail trade path untouched. Zero state
// reads when the ini gate is off (I2: vanilla parity when off).
BOOL __stdcall VPSHARED_ShouldInterceptClickButton(struct SGAMEDATA * pGame, struct D2UnitStrc * pUnit, int16_t nButtonId, uint32_t dwParam);

// Toggle handler for the VP button. dwParam = the ABSOLUTE desired page
// (0 personal, 1..100 shared pages 1..100 - two since R75, ten since R82,
// a hundred since R146; anything above the page count is ignored with a
// d2boot mark). Idempotent: requesting the
// already-active page is a no-op. NOTE (audit ROB-07, v1): no dead/trading
// guard is implemented - a toggle is processed regardless of player state.
// Does the full swap for THIS character: snapshot + dematerialize the
// current page-4 set, materialize the requested page of the character's
// account pool, resync the client (the removal packets ride
// ITEM_RemoveItemFromCurrentLocation; the additions are pushed with
// Send0x9CItemPutInContainer), swap the goldbank borrow on
// personal<->shared edges, and on shared->personal also writes the
// account's .sss file (all hundred pages, v2 format).
void __stdcall VPSHARED_OnClickButton(struct SGAMEDATA * pGame, struct D2UnitStrc * pUnit, int16_t nButtonId, uint32_t dwParam);

// Park the shared page before any .d2s is built (I1). Called from the thin
// VP site at the top of PlayerSave::SaveGameAllGameTypes. When THIS
// character holds a shared page: dematerialize the shared set into the
// account's .sss (with the shared gold absorbed out of UNITSTAT_goldbank),
// restore the personal set + the personal goldbank snapshot, so
// MakeSaveFile serializes retail-only bytes. When the character holds no
// shared page this is a zero-op (no file IO, no marks) - other players'
// saves never touch this account's pool.
void __stdcall VPSHARED_FlushBeforeSave(struct SGAMEDATA * pGame, struct D2UnitStrc * pUnit);

// [R204] Re-activate the shared page for its holder after a save. Called
// from the thin VP site at the END of PlayerSave::SaveGameAllGameTypes:
// the park above is per-save (the .d2s must stay retail-only), and realm
// saves fire per-player on intervals/waypoints - without this hook every
// such save would kick the holder back to their personal page.
void __stdcall VPSHARED_AfterSave(struct SGAMEDATA * pGame, struct D2UnitStrc * pUnit);
