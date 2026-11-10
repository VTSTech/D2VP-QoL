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

#endif  // D2_HAND_H
