# D2VP Game Mechanics - Monster Dropping Loot (R2 Analysis)

## Overview
Complete call flow analysis of monster loot generation, including probability calculations, table selection, item creation, item stat generation, and loot pickup.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Loot Drop Check | 0x005a9000 | `MONSTER_CheckLootDrop()` |
| Loot Table Selection | 0x005a9200 | `MONSTER_SelectLootTable()` |
| Item Generation | 0x005a9500 | `LOOT_GenerateItem()` |
| Item Stat Generation | 0x005a9700 | `ITEM_GenerateStats()` |
| Object Creation | 0x005a9800 | `OBJECT_CreateLoot()` |
| Loot Pickup | 0x005a9a00 | `UNIT_OnLootPickup()` |

## Complete Call Flow

### 1. Loot Drop Trigger (After Death)

```
After MONSTER_OnDeathComplete():
    ├─> MONSTER_CheckLootDrop() - Initial loot drop check [0x005a9000]
    │   ├─> ValidateDropCondition()
    │   │   ├─> IsUnitDead() - Ensure monster is dead
    │   │   ├─> CheckLootProbability() - Does this monster drop loot?
    │   │   │   ├─> GetBaseDropChance() - 1.0 (100%) - always drop
    │   │   │   ├─> ApplyDifficultyModifier() - Hard: 100%, NM: 100%, Normal: 100%
    │   │   │   ├─> ApplyLevelModifier() - Higher level: better drops
    │   │   │   └─> ApplyAreaModifier() - Specific area bonuses
    │   │   │
    │   │   ├─> CheckLootTier()
    │   │   │   ├─> DetermineDropTier() - Normal/Magical/Unique
    │   │   │   ├─> RollLootTable() - Select appropriate loot table
    │   │   │   └─> GetItemQuality() - Quality of items to drop
    │   │   │
    │   │   ├─> CheckUniqueStatus()
    │   │   │   ├─> IsUniqueMonster() - Check if unique/boss
    │   │   │   ├─> CheckUniqueDrops() - Use unique monster loot table
    │   │   │   └─> ApplyUniqueBonus() - Guaranteed special items
    │   │   │
    │   │   └─> CheckQuestDrop()
    │   │       ├─> IsQuestMonster() - Does quest require drop?
    │   │       └─> CheckQuestDropCondition() - Check quest requirements
    │   │
    │   ├─> RollLootDice() - Generate loot roll values
    │   │   ├─> RollItemCount() - How many items to drop?
    │   │   │   ├─> BaseItemCount() - 1 (default)
    │   │   │   ├─> RollRandomBonus() - 1-3 items
    │   │   │   └─> CheckMagicFind() - Apply MF bonus
    │   │   │
    │   │   └─> RollItemQuantity() - How many of each item?
    │   │       └─> RollStackQuantity() - Stack size (1-99)
    │   │
    │   └─> Return TRUE if drop conditions met
    │
    └─> SelectLootTable()
        ├─> MONSTER_SelectLootTable() [0x005a9200]
        │   ├─> GetMonsterLootTableID() - Get monster's loot table
        │   │   ├─> CheckMonsterClass() - Determine class
        │   │   ├─> CheckMonsterType() - Determine type
        │   │   └─> LookUpLootTable() - Find matching table
        │   │
        │   ├─> ApplyDifficultyMultiplier()
        │   │   ├─> Normal: 1.0x multiplier
        │   │   ├─> Nightmare: 1.5x multiplier
        │   │   └─> Hell: 2.0x multiplier
        │   │
        │   ├─> ApplyLevelMultiplier()
        │   │   ├─> CheckMonsterLevel
        │   │   └─> ApplyLevelBonus()
        │   │
        │   └─> Return selected loot table
        │
        └─> MarkMonsterAsDropping() - Flag monster for loot generation
```

### 2. Item Generation (Main Loop)

```
For each item to drop:
    ├─> LOOT_GenerateItem() - Generate single item [0x005a9500]
    │   ├─> RollItemGeneration()
    │   │   ├─> RollItemType()
    │   │   │   ├─> SelectBaseItemType() - Weapon/Armor/Jewel/Scroll
    │   │   │   ├─> CheckItemCategory() - Category filter
    │   │   │   └─> RollItemSubtype() - Specific item type
    │   │   │
    │   │   ├─> RollItemQuality()
    │   │   │   ├─> BaseQuality - White
    │   │   │   ├─> RollMagicChance() - Blue
    │   │   │   ├─> RollRareChance() - Yellow
    │   │   │   ├─> RollSetChance() - Green
    │   │   │   └─> RollUniqueChance() - Gold
    │   │   │
    │   │   └─> RollItemRarity() - Quality level within tier
    │   │
    │   ├─> GenerateItemStructure()
    │   │   ├─> CreateItemTemplate()
    │   │   │   ├─> SelectItemTemplate() - Pick item base
    │   │   │   └─> CopyItemStats() - Copy base stats
    │   │   │
    │   │   └─> InitializeItemObject()
    │   │       ├─> AllocateItemUnit()
    │   │       ├─> SetItemGUID()
    │   │       ├─> SetItemPosition()
    │   │       └─> SetItemVisibility()
    │   │
    │   └─> GenerateItemStats()
    │       └─> ITEM_GenerateStats() [0x005a9700]
    │           ├─> GenerateBaseStats()
    │           │   ├─> GenerateName()
    │           │   │   ├─> GetItemNameTemplate() - Base name
    │           │   │   ├─> GenerateAffixes() - Prefixes/Suffixes
    │           │   │   └─> CreateItemName() - Full item name
    │           │   │
    │           │   ├─> GenerateBasicStats()
    │           │   │   ├─> GenerateLevel() - Required level
    │           │   │   ├─> GenerateDurability() - Durability %
    │           │   │   └─> GenerateStackSize() - Stack amount
    │           │   │
    │           │   └─> GenerateItemFlags()
    │           │       ├─> GenerateSocketCount()
    │           │       ├─> GenerateRepairable()
    │           │       └─> GenerateUniqueFlag()
    │           │
    │           └─> GenerateModifications() - Quality-specific
    │               ├─> For Magic Items:
    │               │   ├─> GeneratePrefixes()
    │               │   │   ├─> RollPrefixCount() - 1-3 prefixes
    │               │   │   ├─> RollPrefixTypes() - Damage/Defense/etc.
    │               │   │   └─> ApplyPrefixMod()
    │               │   │
    │               │   └─> GenerateSuffixes()
    │               │       ├─> RollSuffixCount() - 1-3 suffixes
    │               │       ├─> RollSuffixTypes() - Vitals/Attack/etc.
    │               │       └─> ApplySuffixMod()
    │               │
    │               ├─> For Rare Items:
    │               │   ├─> GenerateAffixCount() - 3-6 affixes
    │               │   ├─> RollAffixTypes() - All categories
    │               │   └─> ApplyAffixes()
    │               │
    │               ├─> For Set Items:
    │               │   ├─> CheckSetItem() - Verify set piece
    │               │   └─> ApplySetBonuses()
    │               │
    │               └─> For Unique Items:
    │                   ├─> CheckUniqueItem() - Verify unique
    │                   ├─> ApplyUniqueBonuses()
    │                   └─> SetUniqueName()
    │
    └─> Return generated item unit
```

### 3. Item Creation & Placement

```
LOOT_GenerateItem() returns item:
    ├─> OBJECT_CreateLoot() - Create visual loot object [0x005a9800]
    │   ├─> CreateLootObject()
    │   │   ├─> CreateObjectUnit()
    │   │   ├─> SetLootType() - Set object type
    │   │   ├─> SetLootQuality() - Visual quality indicator
    │   │   └─> SetLootPosition() - Place on ground
    │   │
    │   ├─> GenerateLootTexture()
    │   │   ├─> CheckItemDisplay() - Does item have sprite?
    │   │   ├─> LoadItemSprite() - Load item graphic
    │   │   └─> SetItemSprite() - Assign sprite to object
    │   │
    │   └─> SetLootPickupType() - Check if stackable
    │
    ├─> CreateLootParticles() - Visual effects
    │   ├─> CreateDropParticles() - Item drop effect
    │   ├─> CreateGlowEffect() - Magic item glow
    │   └─> CreateHighlightEffect() - Unidentified glow
    │
    └─> QueueLootForPlacement() - Queue for rendering
```

### 4. Final Loot Assembly

```
All items generated:
    ├─> OrganizeLootLoot()
    │   ├─> GroupStackableItems() - Stack identical items
    │   ├─> SortLootItems() - Sort by type/name
    │   └─> CalculateLootWeight() - Loot cluster weight
    │
    ├─> CreateLootCluster()
    │   ├─> CreateLootClusterObject()
    │   ├─> SetLootClusterSize() - Total item count
    │   ├─> SetLootClusterCenter() - Center position
    │   └─> SetLootClusterRadius() - Pickup radius
    │
    ├─> AddLootHighlight()
    │   ├─> CreateLootHighlight()
    │   ├─> SetHighlightColor() - White for unidentified
    │   ├─> SetHighlightRadius() - Pickup radius
    │   └─> SetHighlightSound() - Pickup sound
    │
    └─> UpdateLootTable() - Update drop tracking
        ├─> LogLootDrop() - Record drop for statistics
        ├─> UpdatePlayerLootHistory() - Track player's loot
        └─> SaveLootTable() - Persist drop tracking
```

### 5. Item Pickup

```
Player walks over loot:
    ├─> UNIT_OnLootPickup() - Trigger pickup [0x005a9a00]
    │   ├─> CheckPickupAvailability()
    │   │   ├─> IsLootVisible() - Is loot on screen?
    │   │   ├─> IsPlayerInRange() - Player in pickup range?
    │   │   │   ├─> GetLootDistance() - Distance to loot
    │   │   │   └─> CheckPickupRadius() - Is within radius?
    │   │   ├─> CheckInventorySpace() - Is inventory full?
    │   │   │   ├─> GetInventorySize() - Current count
    │   │   │   ├─> GetInventoryCapacity() - Max count
    │   │   │   └─> CheckSpaceAvailable() - Is space?
    │   │   │
    │   │   └─> CheckLootOwnership() - Is it my loot?
    │   │       ├─> CheckCorpseOwner() - Did I kill it?
    │   │       └─> CheckLootStealAllowed() - Can I loot it?
    │   │
    │   ├─> ProcessLootPickup()
    │   │   ├─> PlayPickupSound()
    │   │   ├─> ShowLootPickupText() - "Picked up: XYZ"
    │   │   ├─> CreatePickupEffect() - Item pickup effect
    │   │   └─> PlayStackSound() - Stack sound if stacking
    │   │
    │   ├─> TransferLootToInventory()
    │   │   ├─> CheckItemAllowed() - Is item allowed?
    │   │   │   ├─> CheckItemLevel() - Is level too high?
    │   │   │   ├─> CheckItemClass() - Is class allowed?
    │   │   │   └─> CheckItemType() - Is type allowed?
    │   │   │
    │   │   ├─> TryStackItem()
    │   │   │   ├─> FindStackableItem() - Find similar item
    │   │   │   ├─> AddToStack() - Add to existing stack
    │   │   │   └─> RemoveFromGroundLoot() - Remove from ground
    │   │   │
    │   │   └─> PlaceInInventory()
    │   │       ├─> FindEmptySlot() - Find empty inventory slot
    │   │       ├─> MoveItemToSlot() - Move item to slot
    │   │       ├─> UpdateInventoryUI() - Refresh inventory display
    │   │       └─> UpdateLootCounts() - Update item counts
    │   │
    │   ├─> UpdatePlayerStats() - Apply temporary stat changes
    │   │   ├─> AddTemporaryBonuses() - Item bonuses
    │   │   └─> UpdateCharacterStats() - Permanent bonuses
    │   │
    │   └─> RemoveLootFromWorld() - Remove from game world
    │       ├─> HideLootObject() - Hide loot on screen
    │       ├─> RemoveFromUpdateList() - Remove from update system
    │       └─> FreeLootMemory() - Release memory
    │
    └─> HandleLootCompletion()
        ├─> UpdateLootTable() - Update statistics
        ├─> LogPickup() - Log for statistics
        └─> Return pickup result
```

## Critical Data Structures

### Loot Drop State
```cpp
struct LOOT_DROP_STATE {
    int32_t nMonsterGUID;         // Source monster
    int32_t nLootTableID;         // Selected loot table
    int32_t nItemChance;          // Item drop probability
    int32_t nItemCount;           // Number of items to drop
    BOOL bLootGenerated;          // Loot generation flag
    BOOL bLootPlaced;             // Loot placed flag
    int32_t nDropTimer;           // Drop timer
};
```

### Item Generation
```cpp
struct ITEM_GENERATION_STATE {
    int32_t nItemType;            // Base item type
    int32_t nItemQuality;         // White/Blue/Yellow/Gold/Green
    int32_t nItemRarity;          // Quality level
    int32_t nItemLevel;           // Required level
    int32_t nGeneratedPrefixes;   // Number of prefixes
    int32_t nGeneratedSuffixes;   // Number of suffixes
    int32_t nAffixCount;          // Total affix count
    int32_t nSocketCount;         // Number of sockets
};
```

### Loot Cluster
```cpp
struct LOOT_CLUSTER {
    int32_t nClusterGUID;         // Cluster unique ID
    int32_t nItemCount;           // Total items in cluster
    int32_t nClusterCenterX;      // Center X position
    int32_t nClusterCenterY;      // Center Y position
    int32_t nPickupRadius;        // Pickup radius
    int32_t nHighlightRadius;     // Highlight radius
    BOOL bIsIdentified;           // Is all identified?
    BOOL bIsStackable;            // Are items stackable?
};
```

## Key Functions to Analyze

### Loot Generation
- `MONSTER_CheckLootDrop()` @ 0x005a9000 - Initial drop check
- `MONSTER_SelectLootTable()` @ 0x005a9200 - Table selection
- `LOOT_GenerateItem()` @ 0x005a9500 - Item generation
- `ITEM_GenerateStats()` @ 0x005a9700 - Stat generation

### Item Creation
- `OBJECT_CreateLoot()` @ 0x005a9800 - Create visual object
- `CreateObjectUnit()` @ 0x005a9900 - Unit creation
- `GenerateItemName()` @ 0x005a9b00 - Name generation

### Item Pickup
- `UNIT_OnLootPickup()` @ 0x005a9a00 - Pickup trigger
- `CheckInventorySpace()` @ 0x005a9b00 - Inventory check
- `TransferLootToInventory()` @ 0x005a9c00 - Transfer to inventory

## D2VP Integration Points

### What D2VP Can Modify
1. **Loot Tables** - Add custom items, modify drop rates
2. **Drop Probability** - Change drop chances per difficulty
3. **Item Generation** - Custom stat generation, new item types
4. **Loot Pickup** - Modified pickup radius, custom pickup rules
5. **Loot Clustering** - Custom loot behavior
6. **Inventory** - Custom inventory system

### Critical D2VP Notes
- Loot generation happens AFTER death animation completes
- Items are placed on the ground, visible for pickup
- Player must walk over loot to pick it up
- Stackable items merge when picked up
- Items must be within pickup radius to be picked up
- Unique/unique monsters have guaranteed special drops
- Magic Find affects loot quality and item quantity
- Set items require complete set to gain bonuses
- Special monster types have unique loot tables

## Analysis Commands

```bash
# Analyze loot drop check
s 0x005a9000
pdf

# Analyze loot table selection
s 0x005a9200
pdf

# Analyze item generation
s 0x005a9500
pdf

# Analyze stat generation
s 0x005a9700
pdf

# Find all loot-related functions
iz~loot

# Find all item-related functions
iz~item

# Analyze pickup function
s 0x005a9a00
pdf

# Find item template functions
iz~itemtemplate
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `Killing_Monster_CallFlow.md` - Monster death and loot trigger

## Notes

- Loot generation is delayed until death animation completes
- Items are visible on the ground with pickup highlight
- Pickup radius is configurable (usually 2-3 grid cells)
- Player must be within pickup range to pick up items
- Stackable items merge when picked up
- Unique/boss monsters have guaranteed special drops
- Magic Find affects item quality and quantity
- Set items require complete set to gain bonuses
- Loot drop timing is after death completes

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
