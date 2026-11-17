# D2VP Game Mechanics - Activating a Shrine (R2 Analysis)

## Overview
Complete call flow analysis of shrine interaction, including shrine identification, activation animation, effect application, and state management.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Shrine Hit Detection | 0x00487000 | `OBJECT_OnHit()` - Shrine interaction |
| Shrine Identification | 0x00487200 | `IsShrine()` - Check shrine status |
| Activate Shrine Animation | 0x00487400 | `SHRINE_ActivateAnimation()` |
| Shrine Effect Application | 0x00487600 | `SHRINE_ApplyEffect()` |
| Shrine State Management | 0x00487800 | `SHRINE_UpdateState()` |
| Shrine Name Display | 0x00488000 | `SetShrineTextInfo()` - Name display |

## Complete Call Flow

### 1. Player Interaction with Shrine

```
Player Input (Click/Shrine walk):
    ├─> OBJECT_OnHit() - Interaction trigger [0x00487000]
    │   ├─> CheckInteractionValidity()
    │   │   ├─> IsObjectValid() - Is shrine valid and active?
    │   │   ├─> CheckIsShrine() - Is it a shrine? [0x00487200]
    │   │   │   ├─> IsShrine() - Verify shrine type
    │   │   │   │   ├─> GetChestSubclass() - Check subclass
    │   │   │   │   ├─> CheckShrineTypeFlag() - Is shrine type set?
    │   │   │   │   └─> Return TRUE if shrine
    │   │   │
    │   │   ├─> CheckShrineState()
    │   │   │   ├─> SHRINE_GetState() - Get current state
    │   │   │   ├─> SHRINE_IsActive() - Is shrine active?
    │   │   │   ├─> SHRINE_IsActivated() - Has it been activated?
    │   │   │   └─> SHRINE_IsCooldown() - Is cooldown active?
    │   │   │
    │   │   ├─> CheckPlayerInRange() - Is player near shrine?
    │   │   │   ├─> GetDistanceToShrine() - Calculate distance
    │   │   │   └─> CheckInteractionRadius() - Is within range?
    │   │   │
    │   │   └─> CheckShrineType() - Identify shrine type
    │   │       ├─> GetShrineClass() - Get shrine class ID
    │   │       ├─> GetShrineSubclass() - Get shrine subtype
    │   │       └─> DetermineShrineType() - Health/Mana/XP/Portal/etc.
    │   │
    │   ├─> PlayInteractionSound()
    │   │   ├─> PlayShrineHitSound() - Hit sound
    │   │   └─> CheckShrineType() - Type-specific sound
    │   │
    │   └─> StartShrineActivateSequence()
    │
    └─> If valid interaction:
        └─> SHRINE_ActivateAnimation() - Initiate activation [0x00487400]
            ├─> SaveInteractionState() - Remember interaction
            ├─> PlayActivateSound()
            │   ├─> PlayShrineActivateSound() - Activation sound
            │   └─> CheckShrineType() - Type-specific sound
            │
            ├─> InitiateActivateAnimation()
            │   ├─> SetAnimationState() - Set to ACTIVATING
            │   ├─> SetShrineDirection() - Direction to activate
            │   ├─> SetActivateSpeed() - Animation speed
            │   ├─> SetActivateMode() - Visual mode
            │   └─> StartActivateTimer() - Animation duration
            │
            └─> TriggerActivateVisuals()
                ├─> CreateActivateParticles() - Glow effects
                ├─> CreateActivateEffect() - Shrine activation glow
                └─> UpdateShrineSprite() - Change to activating state
```

### 2. Activate Shrine Animation

```
Shrine Activating Animation:
    ├─> AnimateActivate()
    │   ├─> UpdateActivateProgress() - Track animation progress
    │   ├─> UpdateShrineSprite() - Progressive sprite change
    │   │   ├─> Normal Sprite → Activated Sprite
    │   │   ├─> UpdateGlowEffect() - Update glow intensity
    │   │   ├─> UpdateColorShift() - Color change animation
    │   │   └─> UpdateShineEffect() - Shine effect
    │   │
    │   └─> CheckActivateCompletion()
    │       ├─> UpdateActivateTimer() - Track animation time
    │       ├─> CheckIfActivateComplete() - Is animation done?
    │       └─> TriggerActivateComplete()
    │
    └─> On Activate Complete:
        ├─> SHRINE_OnActivateComplete()
        │   ├─> SetShrineState() - Set to ACTIVATED state
        │   ├─> CloseAnimation() - Close activate animation
        │   ├─> PlayActivateCompleteSound()
        │   ├─> ApplyShrineEffect() - Apply shrine effect
        │   │   └─> SHRINE_ApplyEffect() [0x00487600]
        │   │       ├─> GetShrineType() - Identify effect type
        │   │       ├─> CheckShrineCategory() - Health/Mana/XP/etc.
        │   │       └─> ApplySpecificEffect()
        │   │           ├─> ApplyHealthEffect()
        │   │           │   ├─> SetHP() - Restore health
        │   │           │   ├─> SetMaxHP() - Restore max HP
        │   │           │   ├─> CreateHPEffect() - HP effect particles
        │   │           │   └─> PlayHPSound() - Health sound
        │   │           │
        │   │           ├─> ApplyManaEffect()
        │   │           │   ├─> SetMana() - Restore mana
        │   │           │   ├─> SetMaxMana() - Restore max mana
        │   │           │   ├─> CreateManaEffect() - Mana effect particles
        │   │           │   └─> PlayManaSound() - Mana sound
        │   │           │
        │   │           ├─> ApplyXPEffect()
        │   │           │   ├─> AddExperience() - Grant XP
        │   │           │   ├─> UpdateLevel() - Level up check
        │   │           │   ├─> CreateXPEffect() - XP effect particles
        │   │           │   └─> PlayXPSound() - XP sound
        │   │           │
        │   │           └─> ApplyPortalEffect()
        │   │               ├─> CreatePortal() - Create portal
        │   │               ├─> SetPortalPosition() - Portal location
        │   │               ├─> SetPortalDestination() - Portal destination
        │   │               └─> PlayPortalSound() - Portal sound
        │   │
        │   └─> SetShrineName() - Display shrine name
        │       └─> SetShrineTextInfo() [0x00488000]
        │           ├─> GetShrineCode() - Get shrine code
        │           ├─> GetShrineName() - Get shrine name from text
        │           ├─> DisplayShrineName() - Show name
        │           └─> DisplayShrineEffect() - Show effect type
        │
        └─> TriggerActivateEffects()
            ├─> CreateActivateEffects()
            │   ├─> CreateGlowEffect() - Persistent glow
            │   ├─> CreateEffectParticles() - Effect particles
            │   └─> CreateShineEffect() - Shine effect
            │
            └─> UpdateShrineVisuals()
                ├─> SetShrineToActivated() - Set final activated sprite
                ├─> SetShrineHighlight() - Highlight shrine
                └─> SetShrinePickupRange() - Set interaction range
```

### 3. Shrine Name & Effect Display

```
SetShrineTextInfo() - Set shrine display [0x00488000]
    ├─> GetShrineCode()
    │   ├─> GetShrineSubclass() - Get shrine subtype
    │   ├─> CalculateShrineCode() - Subtype → code mapping
    │   │   ├─> GetShrineTypeMapping() - Use shrine type table
    │   │   └─> Return shrine code (1-23)
    │   │
    │   └─> GetShrineNameFromCode()
    │       ├─> GetShrineNameTable() - Load shrines.txt
    │       ├─> FindShrineRecord() - Find shrine by code
    │       └─> GetShrineName() - Get display name
    │
    ├─> GetShrineDescription()
    │   ├─> GetShrineDescriptionText() - Get description
    │   └─> DisplayDescription() - Show description
    │
    └─> UpdateShrineUI()
        ├─> SetShrineNameDisplay() - Update shrine name
        ├─> SetShrineEffectDisplay() - Update effect type
        └─> SetShrineStatusDisplay() - Update status
```

### 4. Shrine State Management

```
SHRINE_UpdateState() - Update shrine state [0x00487800]
    ├─> CheckShrineState()
    │   ├─> SHRINE_GetState() - Get current state
    │   ├─> CheckIfActivated() - Has it been activated?
    │   ├─> CheckIfEffectApplied() - Has effect been applied?
    │   └─> CheckIfCooldown() - Is cooldown active?
    │
    ├─> HandleCooldownSystem()
    │   ├─> SetShrineCooldown() - Set cooldown timer
    │   │   ├─> CalculateCooldownDuration() - Base + modifiers
    │   │   ├─> ApplyCooldownModifier() - Area/difficulty/quest
    │   │   └─> SetCooldownTimer() - Set countdown
    │   │
    │   ├─> CheckCooldownComplete() - Is cooldown done?
    │   │   ├─> UpdateCooldownTimer() - Track countdown
    │   │   └─> ResetShrineState() - Reset to inactive
    │   │
    │   └─> On Cooldown Complete:
    │       ├─> SetShrineToInactive() - Set to inactive
    │       ├─> HideShrineEffect() - Hide effects
    │       ├─> RemoveShrineHighlight() - Remove highlight
    │       └─> UpdateShrineVisuals() - Reset to normal
    │
    └─> HandleShrineAvailability()
        ├─> CheckShrineAvailability() - Is shrine usable?
        │   ├─> CheckQuestRestriction() - Quest requirement
        │   ├─> CheckAreaRestriction() - Area restriction
        │   └─> CheckShrineTypeRestriction() - Type restriction
        │
        └─> UpdateShrineUIStatus()
            ├─> SetShrineStatusText() - Update status text
            └─> UpdateShrineInteractability() - Enable/disable
```

### 5. Special Shrine Types

```
For Portal Shrines:
    ├─> CheckPortalShrineType()
    │   └─> IsPortalShrine() - Is this a portal shrine?
    │
    ├─> CreatePortalShrineEffect()
    │   ├─> CreatePortalEffect()
    │   │   ├─> CreatePortalObject() - Create portal object
    │   │   ├─> SetPortalDestination() - Portal destination
    │   │   ├─> SetPortalDirection() - Portal facing
    │   │   └─> SetPortalAnimation() - Portal animation
    │   │
    │   └─> SetPortalInteractability()
    │       ├─> AllowPortalInteraction() - Enable portal click
    │       └─> ShowPortalLabel() - Show portal label
    │
    └─> HandlePortalUsage()
        ├─> CheckPortalReadiness() - Is portal ready?
        ├─> ProcessPortalEntry() - Player enters portal
        │   ├─> SetPlayerDestination() - Move player
        │   ├─> TeleportPlayer() - Teleport to portal destination
        │   └─> UpdatePlayerPosition() - Update position
        │
        └─> HandlePortalExit()
            ├─> CreateExitPortal() - Create exit portal
            └─> AllowPortalExits() - Enable exit

For Fiery Death Shrines:
    ├─> CheckFieryDeathShrine()
    │   └─> IsFieryDeathShrine() - Check special type
    │
    ├─> ApplyFieryDeathEffect()
    │   ├─> ApplyDeathEffect() - Death damage
    │   ├─> CreateFieryEffect() - Fiery death effect
    │   └─> ApplyUniqueBonuses() - Unique bonuses

For Gloaming Shrines:
    ├─> CheckGloamingShrine()
    │   └─> IsGloamingShrine() - Check special type
    │
    ├─> ApplyGloamingEffect()
    │   ├─> ApplyGloamingEffect() - Gloaming effect
    │   ├─> CreateGloamingEffect() - Gloaming visual
    │   └─> ProcessGloamingName() - Process name (reversal)
        └─> UpdateGloamingUI() - Update UI
```

## Critical Data Structures

### Shrine State
```cpp
struct SHRINE_STATE {
    int32_t nShrineGUID;          // Shrine unique ID
    int32_t nShrineClass;         // Shrine class ID (68-90)
    int32_t nShrineSubclass;      // Shrine subtype (0-22)
    int32_t nShrineCode;          // Shrine code (1-23)
    int32_t nState;               // INACTIVE/ACTIVATING/ACTIVATED
    int32_t nActivateProgress;    // Animation progress (0-100)
    int32_t nCooldownTimer;       // Cooldown countdown
    int32_t nEffectDuration;      // Effect duration timer
    BOOL bEffectApplied;          // Effect applied flag
    BOOL bCooldownActive;         // Cooldown active flag
};
```

### Shrine Effect Data
```cpp
struct SHRINE_EFFECT_DATA {
    int32_t nShrineGUID;          // Source shrine
    int32_t nShrineType;          // Effect type (Health/Mana/XP/Portal)
    int32_t nEffectValue;         // Effect amount
    int32_t nEffectDuration;      // Effect duration
    int32_t nEffectTimer;         // Effect timer
    int32_t nEffectApplicationCount; // Times applied
};
```

## Key Functions to Analyze

### Shrine Interaction
- `OBJECT_OnHit()` @ 0x00487000 - Shrine hit detection
- `IsShrine()` @ 0x00487200 - Shrine identification
- `SHRINE_ActivateAnimation()` @ 0x00487400 - Activate animation

### Shrine Effects
- `SHRINE_ApplyEffect()` @ 0x00487600 - Apply effect
- `ApplyHealthEffect()` @ 0x00487700 - Health effect
- `ApplyManaEffect()` @ 0x00487800 - Mana effect
- `ApplyXPEffect()` @ 0x00487900 - XP effect

### Shrine Display
- `SetShrineTextInfo()` @ 0x00488000 - Set shrine name
- `GetShrineCode()` @ 0x00488100 - Get shrine code
- `GetShrineName()` @ 0x00488200 - Get shrine name

### Shrine State
- `SHRINE_UpdateState()` @ 0x00487800 - State management
- `SetShrineCooldown()` @ 0x00487900 - Cooldown system
- `SHRINE_OnActivateComplete()` @ 0x00488000 - Activate completion

## D2VP Integration Points

### What D2VP Can Modify
1. **Shrine Identification** - Custom shrine types, custom mapping
2. **Shrine Effects** - Modify effects, add new effects
3. **Shrine Cooldown** - Change cooldown duration, system
4. **Shrine Visuals** - Custom sprites, effects
5. **Shrine Names** - Custom name display, localization
6. **Portal Shrines** - Custom portal behavior

### Critical D2VP Notes
- Shrine activation requires walking into shrine
- Activation animation plays before effect application
- Shrine name displays after activation completes
- Cooldown prevents re-activation during cooldown
- Portal shrines create teleport portals
- Effect type determined by shrine code
- Shrine types: Health, Mana, XP, Stamina, Portal
- Special shrine types: Fiery Death, Gloaming
- All shrine types have same activation flow

## Analysis Commands

```bash
# Analyze shrine hit detection
s 0x00487000
pdf

# Analyze shrine identification
s 0x00487200
pdf

# Analyze shrine activation animation
s 0x00487400
pdf

# Find all shrine-related functions
iz~shrine

# Analyze shrine effect application
s 0x00487600
pdf

# Analyze shrine name display
s 0x00488000
pdf

# Analyze shrine state management
s 0x00487800
pdf
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `Opening_Chest_CallFlow.md` - Similar object interaction flow
- `shrine-parameter-typing-fix-r167-complete.md` - Shrine parameter fix

## Notes

- Shrine activation is triggered by walking into shrine
- Activation animation plays before effect application
- Shrine name displays after activation completes
- Cooldown prevents re-activation during cooldown
- Portal shrines create teleport portals
- Effect type determined by shrine code
- All shrine types have same activation flow
- Special shrine types: Fiery Death, Gloaming
- Shrine effects are immediate upon activation

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
- **R167-R169**: Shrine parameter and type mapping fixes documented
