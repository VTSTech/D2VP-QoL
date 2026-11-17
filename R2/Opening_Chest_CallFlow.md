# D2VP Game Mechanics - Opening a Chest (R2 Analysis)

## Overview
Complete call flow analysis of chest interaction, including collision detection, opening animation, loot generation, item spawning, and chest state management.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Chest Hit Detection | 0x00486000 | `OBJECT_OnHit()` - Chest interaction |
| Open Chest Animation | 0x00486200 | `CHEST_OpenAnimation()` |
| Chest Loot Generation | 0x00486400 | `CHEST_GenerateLoot()` |
| Chest Item Placement | 0x00486600 | `CHEST_PlaceLoot()` |
| Chest State Management | 0x00486800 | `CHEST_UpdateState()` |

## Complete Call Flow

### 1. Player Interaction with Chest

```
Player Input (Click/Dash into chest):
    ├─> OBJECT_OnHit() - Interaction trigger [0x00486000]
    │   ├─> CheckInteractionValidity()
    │   │   ├─> IsObjectValid() - Is chest valid and active?
    │   │   ├─> CheckIsOpenable() - Is chest open/closeable?
    │   │   ├─> CheckChestState() - Is it open or closed?
    │   │   │   ├─> CHEST_GetState() - Get current state
    │   │   │   ├─> CHEST_IsOpen() - Check if open
    │   │   │   └─> CHEST_IsLocked() - Check if locked
    │   │   │
    │   │   ├─> CheckPlayerInRange() - Is player near chest?
    │   │   │   ├─> GetDistanceToChest() - Calculate distance
    │   │   │   └─> CheckInteractionRadius() - Is within range?
    │   │   │
    │   │   └─> CheckOwnership() - Is it my chest?
    │   │       ├─> CheckChestOwner() - Who owns chest?
    │   │       └─> AllowInteract() - Is it allowed?
    │   │
    │   ├─> PlayInteractionSound()
    │   │   ├─> PlayChestHitSound() - Hit sound
    │   │   └─> CheckChestType() - Special chest sounds
    │   │
    │   ├─> CheckChestType() - Identify chest type
    │   │   ├─> GetChestClass() - Get chest class ID
    │   │   ├─> GetChestSubclass() - Get chest subclass
    │   │   └─> DetermineChestType() - Normal/Magic/Unique
    │   │
    │   └─> StartChestOpenSequence()
    │
    └─> If valid interaction:
        └─> CHEST_OpenAnimation() - Initiate opening [0x00486200]
            ├─> SaveInteractionState() - Remember interaction
            ├─> PlayOpenSound()
            │   ├─> PlayChestOpenSound() - Open sound
            │   └─> CheckChestType() - Special open sound
            │
            ├─> InitiateOpenAnimation()
            │   ├─> SetAnimationState() - Set to OPENING
            │   ├─> SetChestDirection() - Direction to open
            │   ├─> SetOpenSpeed() - Animation speed
            │   └─> StartOpenTimer() - Animation duration
            │
            └─> TriggerOpenVisuals()
                ├─> CreateOpenParticles() - Dust/sparkle
                ├─> CreateOpenEffect() - Chest swing animation
                └─> UpdateChestSprite() - Change to open state
```

### 2. Open Chest Animation

```
Chest Opening Animation:
    ├─> AnimateOpen()
    │   ├─> UpdateOpenProgress() - Track animation progress
    │   ├─> UpdateChestSprite() - Progressive sprite change
    │   │   ├─> Closed Sprite → Open Sprite
    │   │   ├─> UpdateChestPosition() - Shift chest position
    │   │   └─> UpdateChestRotation() - Rotate for swing
    │   │
    │   └─> CheckOpenCompletion()
    │       ├─> UpdateOpenTimer() - Track animation time
    │       ├─> CheckIfOpenComplete() - Is animation done?
    │       └─> TriggerOpenComplete()
    │
    └─> On Open Complete:
        ├─> CHEST_OnOpenComplete()
        │   ├─> SetChestState() - Set to OPEN state
        │   ├─> CloseAnimation() - Close open animation
        │   ├─> PlayOpenCompleteSound()
        │   └─> GenerateLoot() - Start loot generation
        │
        └─> TriggerOpenEffects()
            ├─> CreateOpenEffects()
            │   ├─> CreateLootParticle() - Item drop particles
            │   ├─> CreateLootGlow() - Magic item glow
            │   ├─> CreateLootHighlight() - Pickup highlight
            │   └─> PlayLootSound() - Loot appear sound
            │
            └─> UpdateChestVisuals()
                ├─> SetChestToOpen() - Set final open sprite
                ├─> ShowChestLoot() - Show loot indicator
                └─> SetChestPickupRange() - Set pickup radius
```

### 3. Loot Generation

```
CHEST_GenerateLoot() - Generate chest loot [0x00486400]
    ├─> ValidateLootGeneration()
    │   ├─> CheckChestType() - Is it a normal chest?
    │   ├─> CheckChestLootTable() - Does chest have loot table?
    │   ├─> CheckLootCount() - Max loot per chest?
    │   └─> CheckChestCooldown() - Is there a cooldown?
    │
    ├─> SelectChestLootTable()
    │   ├─> GetChestLootTable() - Get chest's loot table
    │   │   ├─> CheckChestClass() - Class-based loot table
    │   │   ├─> CheckChestSubclass() - Subclass-specific table
    │   │   └─> ApplyDifficultyModifier() - Difficulty bonus
    │   │
    │   └─> ApplyLootModifiers()
    │       ├─> CheckAreaModifier() - Area-based bonus
    │       ├─> CheckQuestModifier() - Quest chest bonus
    │       └─> ApplyMagicFind() - Player MF bonus
    │
    └─> Return selected loot table
```

### 4. Item Placement & Rendering

```
CHEST_PlaceLoot() - Place loot items [0x00486600]
    ├─> ProcessLootItems()
    │   ├─> CheckItemQuantity() - How many items?
    │   │   ├─> GetMaxItemsPerChest() - Check chest limit
    │   │   ├─> RollRandomItems() - Random item count
    │   │   └─> RollSpecialItems() - Special item chance
    │   │
    │   └─> For each item:
    │       ├─> GenerateItem() - Use same generation as monster loot
    │       │   └─> ITEM_GenerateItem() - Generate item structure
    │       │
    │       ├─> PlaceItemInChest()
    │       │   ├─> CreateLootObject() - Create visual item
    │       │   ├─> SetLootPosition() - Place near chest
    │       │   ├─> SetLootOrientation() - Item facing
    │       │   └─> SetLootCluster() - Add to chest cluster
    │       │
    │       └─> AddLootHighlight()
    │           ├─> CreateLootHighlight() - Show item
    │           ├─> SetHighlightColor() - White (unidentified)
    │           └─> SetHighlightRadius() - Pickup radius
    │
    └─> CreateChestLootCluster()
        ├─> CreateLootClusterObject()
        ├─> SetClusterCenter() - Center of chest
        ├─> SetClusterRadius() - Pickup radius
        └─> SetClusterItemCount() - Total items
```

### 5. Chest State Management & Cleanup

```
CHEST_UpdateState() - Update chest state [0x00486800]
    ├─> CheckChestState()
    │   ├─> CHEST_GetState() - Get current state
    │   ├─> CheckIfLootGenerated() - Has loot been generated?
    │   └─> CheckIfAllPickedUp() - Are all items picked up?
    │
    ├─> HandlePickedUpItems()
    │   ├─> TrackItemPickup() - Track items remaining
    │   ├─> CheckRemainingItems()
    │   │   ├─> GetRemainingItemCount() - Count items left
    │   │   ├─> CheckIsClusterComplete() - All picked up?
    │   │   └─> TriggerClusterCleanup()
    │   │
    │   └─> If all picked up:
    │       ├─> RemoveChestLoot() - Remove loot items
    │       ├─> HideLootHighlight() - Hide item highlight
    │       ├─> UpdateChestVisuals()
    │       │   ├─> SetChestToClosed() - Reset to closed
    │       │   ├─> HideLootIndicator() - Hide loot icon
    │       │   └─> ResetChestState() - Reset state
    │       │
    │       ├─> PlayChestCloseSound()
    │       ├─> TriggerChestClose()
    │       │   ├─> SetAnimationState() - Set to CLOSING
    │       │   ├─> StartCloseAnimation() - Play close anim
    │       │   └─> PlayCloseSound()
    │       │
    │       └─> UpdateChestTimer()
    │           ├─> SetCloseTimer() - Close after delay
    │           └─> MarkChestForCleanup() - Schedule removal
    │
    └─> UpdateChestCooldown()
        ├─> SetChestCooldown() - Cooldown timer
        ├─> CheckCooldownComplete() - Is cooldown done?
        └─> ResetChestCooldown() - Reset on timeout
```

### 6. Chest Close Animation

```
When close timer expires:
    ├─> CHEST_CloseAnimation()
    │   ├─> TriggerChestClose()
    │   │   ├─> SetAnimationState() - Set to CLOSING
    │   │   ├─> StartCloseAnimation() - Play close anim
    │   │   └─> PlayCloseSound()
    │   │
    │   └─> AnimateClose()
    │       ├─> UpdateCloseProgress() - Track animation
    │       ├─> UpdateChestSprite() - Reverse sprite change
    │       └─> CheckCloseCompletion() - Is done?
    │
    └─> On Close Complete:
        ├─> SetChestState() - Set to CLOSED state
        ├─> CloseAnimation() - Close animation
        ├─> RemoveLootParticles() - Clear particle effects
        └─> UpdateChestVisuals() - Finalize closed state
```

## Critical Data Structures

### Chest State
```cpp
struct CHEST_STATE {
    int32_t nChestGUID;           // Chest unique ID
    int32_t nChestClass;          // Chest class ID
    int32_t nChestSubclass;       // Chest subclass
    int32_t nState;               // CLOSED/OPENING/OPEN/CLOSING
    int32_t nOpenProgress;        // Animation progress (0-100)
    int32_t nLootTableID;         // Selected loot table
    int32_t nLootItemCount;       // Total items in chest
    int32_t nRemainingItems;      // Items not yet picked up
    int32_t nCooldownTimer;       // Cooldown countdown
    int32_t nOpenTimer;           // Open animation timer
    int32_t nCloseTimer;          // Close animation timer
};
```

### Chest Loot Data
```cpp
struct CHEST_LOOT_DATA {
    int32_t nChestGUID;           // Source chest
    int32_t nLootClusterID;       // Loot cluster ID
    int32_t nGeneratedItems;      // Number of items generated
    int32_t nRemainingItems;      // Items remaining to pick up
    BOOL bLootGenerated;          // Loot generation flag
    BOOL bAllPickedUp;            // All items picked up flag
    BOOL bIsClosed;               // Chest closed flag
};
```

## Key Functions to Analyze

### Chest Interaction
- `OBJECT_OnHit()` @ 0x00486000 - Chest hit detection
- `CHEST_OpenAnimation()` @ 0x00486200 - Open animation
- `CHEST_OnOpenComplete()` @ 0x00486300 - Open completion

### Chest Logic
- `CHEST_GenerateLoot()` @ 0x00486400 - Loot generation
- `CHEST_PlaceLoot()` @ 0x00486600 - Item placement
- `CHEST_UpdateState()` @ 0x00486800 - State management

### Chest Animation
- `CHEST_AnimateOpen()` @ 0x00486900 - Open animation loop
- `CHEST_AnimateClose()` @ 0x00486a00 - Close animation loop
- `CHEST_UpdateOpenProgress()` @ 0x00486b00 - Animation progress

## D2VP Integration Points

### What D2VP Can Modify
1. **Chest Interaction** - Modify hit detection, pickup radius
2. **Chest Animation** - Custom open/close animations
3. **Chest Loot** - Custom loot tables, special chests
4. **Chest States** - Change chest behavior, locking systems
5. **Chest Visuals** - Custom chest sprites, effects

### Critical D2VP Notes
- Chest interaction is triggered by walking into chest
- Animation plays before loot generation
- Chest must be fully open before loot appears
- Chest closes after a delay if not all items picked up
- Stackable items merge when picked up
- Chest cooldown prevents multiple interactions
- Magic chests have unique loot tables
- Locked chests require keys or special handling
- Special chest types (small/large/rare) have different behavior

## Analysis Commands

```bash
# Analyze chest hit detection
s 0x00486000
pdf

# Analyze chest open animation
s 0x00486200
pdf

# Analyze chest loot generation
s 0x00486400
pdf

# Find all chest-related functions
iz~chest

# Analyze chest state management
s 0x00486800
pdf

# Analyze chest object handling
iz~object
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `Monster_Dropping_Loot_CallFlow.md` - General loot generation

## Notes

- Chest interaction requires player to walk into chest
- Open animation plays before loot generation
- Loot appears only after animation completes
- Chest closes if not all items picked up
- Chest has cooldown after opening
- Stackable items merge when picked up
- Magic chests have unique loot tables
- Special chest types (small/large) have different loot
- Chests can be locked/unlocked
- Chest animation timing is important for player experience

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
