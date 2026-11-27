// Hand-written declarations for entities the extraction provably cannot supply.
//
// Nothing here is invented. Each declaration names a real entity with its real
// signature, sourced as noted per block. This file is never emitted by the
// generator; it reaches a translation unit only through a forced include
// (`-include Redivivus/d2_hand.h`), so no generated file references it.
#ifndef D2_HAND_H
#define D2_HAND_H

// --- [Vanilla+ R152, issue #37] D2Common::INVENTORY::Inventory::
//     INVENTORY_RemoveItemFromInventory -----------------------------------
// Hand re-exposure of a real, unchanged entity. The generated header does
// declare it (Source/D2Common/INVENTORY/Inventory.h :221) but inside the
// "Module-private" D2_MODULE_D2Common guard — the extraction found no retail
// cross-module caller, so the D2Client TU never sees the declaration and the
// R152 build broke there (error: not a member of
// D2Common::INVENTORY::Inventory). The R152 client fix needs it from
// D2Client: HandleItemRemoveFromContainer's REMOVED branch unlinks the doomed
// item unit from the owning inventory chain BEFORE the retail free
// (CUnit::RemoveObject -> FreeAny), so the open panels' draw walk can no
// longer reach freed/reused units through the chain (the #37 AV read
// [null+0xC] at DRAW_UI_InventoryContent+0x9A, d2crash 20261109-200257).
// Declared with the exact generated signature; the client call links the
// existing D2Common definition (Inventory.cpp :298). No wrapper, no new code.
// NOTE: this file is force-included before any generated header, so the
// parameter's struct must be forward-declared at global scope HERE — an
// elaborated `struct D2UnitStrc` inside the namespace would otherwise inject
// a wrong D2Common::D2UnitStrc type and change the mangled signature.
// @source Source/D2Common/INVENTORY/Inventory.h :221 (signature
//          byte-identical) and Inventory.cpp :298 (definition).
struct D2UnitStrc;
namespace D2Common { namespace INVENTORY { namespace Inventory {
BOOL __fastcall INVENTORY_RemoveItemFromInventory(struct D2UnitStrc * pItem);
}}} // D2Common::INVENTORY::Inventory

// --- [Vanilla+ R158.3] D2Common::Stats::StatsEx::UNITS_CalcCombinedStatValue
// Same shape as the R152 block above: hand re-exposure of a real, unchanged
// entity. The generated header does declare it (Source/D2Common/Stats/
// StatsEx.h :159) but inside the "Module-private"
// `#if !defined(D2_MODULE_LIBRARIES) || defined(D2_MODULE_D2Common)` guard
// (StatsEx.h :121..181) — the R158.2 field-fix switched the extended-stats
// row loop from SERVER_GetUnitStat (root base array only: every gear stat
// read 0, and the ItemStatCost minaccr clamp showed velocitypercent as 100)
// to this getter, which walks base + every child statlist with no display
// clamp (StatsEx.cpp, via STATLIST_CalcFullStatFromChildren) - and the
// D2Client TU (it defines D2_MODULE_LIBRARIES) never sees the gated
// declaration, so the build broke there (error: 'UNITS_CalcCombinedStatValue'
// is not a member of 'D2Common::Stats::StatsEx', owner build log 2026-11-16).
// Declared with the exact generated signature; the client call links the
// existing D2Common definition (StatsEx.cpp, UNITS_CalcCombinedStatValue).
// No wrapper, no new code.
// NOTE: at this forced-include point no generated header has run yet, so the
// parameter type cannot be named `eD2UnitStat` directly - the type is a
// PLAIN GLOBAL TYPEDEF of uint32_t in d2_enums.h (:14592, the UNITSTAT
// constants live in eD2UnitStat_ns). Spelling the typedef here is the same
// type with the same mangled signature, and a legal redeclaration once the
// generated header typedefs it again; the elaborated-struct trap from the
// R152 note does not apply to a typedef of a fundamental type.
// @source Source/D2Common/Stats/StatsEx.h :159 (signature byte-identical) and
//         StatsEx.cpp (definition, UNITS_CalcCombinedStatValue).
typedef uint32_t eD2UnitStat;
namespace D2Common { namespace Stats { namespace StatsEx {
int32_t __stdcall UNITS_CalcCombinedStatValue(struct D2UnitStrc * pUnit, eD2UnitStat eUnitStat);
}}} // D2Common::Stats::StatsEx

// --- [Vanilla+ R237] C->S 0x38 (NPC menu select) gamble "Refresh" selector --
// Not a declaration: the one constant the fork needs in BOTH halves of the
// client/server split. 0x38's second dword is the menu action (1 trade,
// 2 open-gamble, 3 hire) and its fourth dword (dwParams) is unused by retail
// for action 2 - the client always sends 0. The Refresh button sends this
// value there, so SUNITNPC::UseMenu can tell a re-roll request from a plain
// re-open without a new packet id (the C->S dispatch table in PlayerMsg.cpp is
// a fixed array and both ends ship as one build). Retail never sends it, so a
// stock client is unaffected. It lives here because D2Game cannot import
// D2Client and this header is the only non-generated one both already see.
constexpr uint32_t D2VP_GAMBLE_REFRESH_PARAM = 0x00005246; // 'R','F'

#endif  // D2_HAND_H
