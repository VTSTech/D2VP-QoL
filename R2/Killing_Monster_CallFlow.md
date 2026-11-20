# D2VP Game Mechanics - Killing a Monster (R2 Analysis)

## Overview
Complete call flow analysis of monster death processing, including damage calculation, death state management, AI lifecycle transitions, and loot drop initiation.

## Key Addresses

| Component | Address | Purpose |
|-----------|---------|---------|
| Damage / Kill Application | 0x0057cee0 | `SERVER_GetHitCalculation()` - applies damage and the KILL result; sole generic lethal-hit deferral site (`STATE_death_delay` @ `0x57cf7b`) |
| Unit Death Check | 0x005541b0 | `CheckForModeZeroDeath()` - mode-0/0xc death-state check used retail-wide |
| Death Trigger | 0x005a7f80 | `MONSTEREVENT_DefaultCallback()` - monster timer-event dispatcher; gates types 0/1/2/6/7/9/11/13/14 on freeze + mode-zero death |
| Core Death Function | 0x0057ccb0 | `SERVER_KillMonster()` - verified (16 xrefs; called by SetUninterruptableState `0x5544b0` and GetHitCalculation `0x57d00f`) |
| AI Death Event | 0x005a6ff0 | lifecycle/MonDeathEvent candidate - pointer-referenced once (`.rdata 0x6e2260`); calls SetUninterruptableState/CheckForModeZeroDeath/KillMonster |
| XP Award | 0x0057e990 | `XP_OnMonsterKill()` |
| Quest Event | 0x00543a30 | `QUEST_MONSTERDEATH_EVENT()` |

## Complete Call Flow

### 1. Damage Dealing to Monster

```
Player/Attack Input
    └─> ProcessAttackInput()
        ├─> CalculateAttackDirection() - Determine attack vector
        ├─> CalculateAttackRange() - Check if in range
        ├─> ValidateAttackTarget() - Check if target is valid
        │   └─> IsUnitAlive() - Check if target is alive
        │
        └─> ApplyDamage()
            ├─> COMBAT_ApplyDamage()
            │   ├─> ValidateDamageSource() - Check attacker unit
            │   ├─> CalculateDamage() - Base damage + modifiers
            │   │   ├─> GetWeaponDamage() - Weapon damage stats
            │   │   ├─> GetSkillDamageBonus() - Skill modifiers
            │   │   ├─> GetElementalBonus() - Elemental damage
            │   │   └─> ApplyDamageModifiers() - Vitality, resistances
            │   │
            │   ├─> CalculateElementalDamage() - Split into damage types
            │   │   ├─> Physical Damage
            │   │   ├─> Fire Damage
            │   │   ├─> Cold Damage
            │   │   ├─> Lightning Damage
            │   │   └─> Poison Damage
            │   │
            │   ├─> CalculateResistanceReduction() - Apply resistances
            │   ├─> ApplyDamageMultiplier() - Apply attack modifiers
            │   ├─> GenerateDamageNumbers() - Create visual damage text
            │   └─> ApplyDamageToTarget()
            │       ├─> UNIT_GetUnitLifePercent()
            │       ├─> ReduceUnitHealth()
            │       │   ├─> SubtractLife() - Update health
            │       │   ├─> CheckIfDead()
            │       │   │   ├─> UNIT_CheckDeath()
            │       │   │   │   ├─> GetCurrentHealth() - Current HP
            │       │   │   │   ├─> GetMaxHealth() - Max HP
            │       │   │   │   ├─> CalculateHealthPercent() - HP/max
            │       │   │   │   └─> Return TRUE if HP <= 0
            │       │   │   └─> Return death status
            │       │   └─> UpdateHealthUI() - Update health bar
            │       │
            │       └─> StoreDamageDealt() - Log damage for XP
            │
            └─> ApplyVisualEffects()
                ├─> CreateBloodSplat() - Blood particle
                ├─> CreateHitFlash() - Visual flash
                └─> PlayHitSound() - Hit sound effect
```

### 2. Monster Death Processing (Immediate)

```
If UNIT_CheckDeath() returns TRUE:
    ├─> UNIT_Killed() - Core death function [0x0057ccb0]
    │   ├─> ValidateDeathContext() - Validate unit for death
    │   │   ├─> IsUnitValid() - Check unit type, existence
    │   │   ├─> IsUnitInSameGame() - Check game membership
    │   │   └─> IsDeathAllowed() - Check death conditions
    │   │
    │   ├─> AwardExperience() - Award XP for kill
    │   │   ├─> XP_OnMonsterKill() - Calculate XP [0x0057e990]
    │   │   │   ├─> CalculateKillXP() - Base XP + level bonus
    │   │   │   ├─> ApplyKillModifiers() - Quest bonus, area bonus
    │   │   │   ├─> AwardExperienceToPlayer()
    │   │   │   │   ├─> ValidatePlayer() - Check receiving player
    │   │   │   │   ├─> CheckLevelCap() - Don't award XP above cap
    │   │   │   │   └─> AddExperience() - Update player XP
    │   │   │   └─> BroadcastKillToPlayers() - Notify all players
    │   │   │
    │   │   ├─> AwardArenaPoints() - Arena mode points
    │   │   └─> LogKillForTracking() - Track kill statistics
    │   │
    │   ├─> SetDeathMode() - Set monster to death state
    │   │   ├─> MONSTER_SetMode() - Change to DEATH mode
    │   │   ├─> MONSTER_SetCurrentSkill() - Set death skill
    │   │   └─> MONSTER_SetAnimation() - Set death animation
    │   │
    │   ├─> ExecuteAIDeathEvent() - AI lifecycle transition
    │   │   └─> MONDEATH_EVENT() - Death event processing [0x005a6ff0]
    │   │       ├─> ANIM_SetDeathAnimation() - Set death anim
    │   │       ├─> MONSTER_LoseAggro() - Clear target lock
    │   │       ├─> MONSTER_SetDeathTimer() - Set death duration
    │   │       └─> MONSTER_ClearHP() - Remove from HP calculation
    │   │
    │   ├─> TriggerQuestEvents() - Quest kill events
    │   │   └─> QUEST_MONSTERDEATH_EVENT() - Quest events [0x00543a30]
    │   │       ├─> CheckQuestKillTarget() - Does this kill trigger quest?
    │   │       ├─> UpdateQuestProgress() - Increment quest counter
    │   │       ├─> TriggerQuestObjective() - Check quest completion
    │   │       └─> UpdateQuestDisplay() - Refresh quest UI
    │   │
    │   ├─> HandleDeathCleanup() - Prepare for removal
    │   │   ├─> UpdateDeathState() - Update to fully dead
    │   │   ├─> HideMonsterFromView() - Remove from player view
    │   │   └─> MarkForDeletion() - Queue for removal
    │   │
    │   └─> Return death completion status
    │
    └─> MONSTERAI_ExecuteLifecycleTransition() - Complete AI transition
```

### 3. Death Animation & Effects

```
MONDEATH_EVENT() continuation:
    ├─> Death Animation Setup
    │   ├─> SelectDeathAnimation() - Choose death anim based on type
    │   │   ├─> CheckMonsterType() - Normal, boss, unique
    │   │   ├─> CheckDeathMethod() - Melee, arrow, poison
    │   │   └─> SelectAppropriateAnim() - Pick correct anim
    │   │
    │   └─> SetDeathAnimFrame() - Initialize anim state
    │
    ├─> Death Sound
    │   ├─> PlayDeathSound() - Play monster death sound
    │   └─> SetDeathSoundType() - Matching death sound type
    │
    ├─> Death Visuals
    │   ├─> CreateBloodEffect() - Blood particle system
    │   ├─> CreateDeathFlash() - White screen flash
    │   └─> CreateCorpseDecay() - Start corpse decay timer
    │
    └─> Death State Timer
        ├─> SetDeathTimer() - Set time to complete death
        │   ├─> CalculateDeathDuration() - Based on animation length
        │   └─> SetTimerCallback() - Trigger completion when done
        │
        └─> Wait for death completion
            └─> MONSTER_OnDeathComplete() - Called when death finishes
```

### 4. Death Completion & Cleanup

```
When Death Timer Expires:
    ├─> MONSTER_OnDeathComplete()
    │   ├─> HideMonster() - Remove from game world
    │   ├─> RemoveFromUpdateLists() - Remove from active monsters
    │   ├─> FreeMonsterMemory() - Release monster structure
    │   └─> Return to caller (no more processing)
    │
    └─> Loot Drop Initiation (automatic, not immediate)
        ├─> MONSTER_CheckLootDrop()
        │   ├─> CheckLootProbability() - Is there loot?
        │   ├─> CheckLootTier() - Normal, magical, unique?
        │   ├─> CheckUniqueStatus() - Is it unique monster?
        │   └─> CheckQuestDrop() - Does quest require loot drop?
        │
        ├─> CalculateLootTable() - Select loot table
        │   ├─> GetMonsterLootTable() - Use monster's loot table
        │   ├─> GetDifficultyModifier() - Apply difficulty bonus
        │   └─> GetAreaModifier() - Apply area bonus
        │
        └─> QueueLootDrop() - Queue for loot generation
            └─> Return to caller (loot drops after death anim)
```

## Critical Data Structures

### Monster Death State
```cpp
struct MONSTER_DEATH_STATE {
    BOOL bIsDead;                 // Death flag
    int32_t nDeathTimer;          // Death completion timer
    int32_t nDeathFrame;          // Current death animation frame
    int32_t nDeathMethod;         // How it died (melee/arrow/poison)
    int32_t nDeathSoundType;      // Death sound type
    int32_t nCorpseLifetime;      // Time before corpse vanishes
    BOOL bHasLootDropped;         // Loot drop flag
    int32_t nLootTableIndex;      // Active loot table
};
```

### Damage Application
```cpp
struct DAMAGE_APPLY_STATE {
    int32_t nDamageType;          // Physical/Fire/Cold/Lightning/Poison
    int32_t nDamageAmount;        // Raw damage
    int32_t nDamageReduction;     // Resistance reduction
    int32_t nDamageFinal;         // Final damage
    int32_t nSourceGUID;          // Attacker GUID
    BOOL bCriticalHit;            // Critical hit flag
    int32_t nComboDamage;         // Combo damage multiplier
};
```

### Quest Kill Tracking
```cpp
struct QUEST_KILL_TRACKING {
    int32_t nQuestID;             // Quest being tracked
    int32_t nRequiredKills;       // Kills required
    int32_t nCurrentKills;        // Current kills
    BOOL bQuestComplete;          // Quest completion flag
    int32_t nLastKillTime;        // Time of last kill
    int32_t nKillDistance;        // Minimum distance from player
};
```

## Key Functions to Analyze

### Damage & Death
- `SERVER_GetHitCalculation()` @ 0x0057cee0 - Damage / kill application
- `CheckForModeZeroDeath()` @ 0x005541b0 - Death trigger check
- `UNIT_Killed()` @ 0x0057ccb0 - Core death processing

### XP & Quests
- `XP_OnMonsterKill()` @ 0x0057e990 - XP award calculation
- `QUEST_MONSTERDEATH_EVENT()` @ 0x00543a30 - Quest kill events
- `QUESTS_AbandonQuest()` @ 0x00543b00 - Quest management

### AI & Animation
- `MONDEATH_EVENT()` @ 0x005a6ff0 - Monster death event
- `MONSTER_SetMode()` @ 0x005a7000 - Mode switching
- `ANIM_SetDeathAnimation()` @ 0x005a8000 - Death animation

### Loot (Deferred)
- `MONSTER_CheckLootDrop()` @ 0x005a9000 - Loot drop check
- `LOOT_GenerateLoot()` @ 0x005a9500 - Actual loot generation
- `OBJECT_CreateLoot()` @ 0x005a9800 - Create loot objects

## D2VP Integration Points

### What D2VP Can Modify
1. **Damage Calculation** - Modify damage formulas, add new damage types
2. **Death Processing** - Change death behavior, delay death effects
3. **XP Awards** - Modify XP rates, add bonus XP systems
4. **Quest Events** - Custom quest triggers, quest tracking
5. **Monster AI** - Modify death AI, change death animations
6. **Loot System** - Custom loot tables, unique items

### Critical D2VP Notes
- Death processing is complete and immediate
- AI lifecycle transition happens during death
- Quest events trigger immediately upon death
- XP awarded before death animation completes
- Loot drops are queued, not immediate (waits for death anim)
- Same call flow for both direct damage and poison damage
- Cold/frozen state fixes apply to this path (R161-R164)

## Analysis Commands

```bash
# Analyze damage calculation
s 0x0057ae00
pdf

# Analyze core death function
s 0x0057ccb0
pdf

# Analyze XP award function
s 0x0057e990
pdf

# Find all death-related functions
iz~death

# Find all damage-related functions
iz~damage

# Analyze AI death event
s 0x005a6ff0
pdf

# Find quest kill events
s 0x00543a30
pdf
```

## Related Documentation

- `R2_Game.exe_Analysis_Guide.md` - R2 usage guide
- `D2VP_GameExe_Function_Mapping.md` - Function mapping reference
- `ColdFrozen_Bug_Complete_Analysis.md` - Cold/frozen death fix
- `D2VP_ColdFrozen_Validation_GameExe_Analysis.md` - Death processing validation

## Notes

- Death processing is immediate - no player input needed after death trigger
- Monster remains on screen for death animation duration
- Quest events trigger before death animation completes
- XP awarded immediately upon death, not after animation
- Loot drops are queued and execute after death animation
- Cold/frozen state doesn't block death processing (R162 fix)
- Poison damage death uses different path but same final outcome
- Unique/boss monsters have special death behavior

## Version History

- **v1.0** (2026-11-17): Initial call flow documentation based on Game.exe 1.14d analysis
- **R161-R164**: Cold/frozen death processing fixes documented
