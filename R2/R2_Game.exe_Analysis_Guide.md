# R2 Game.exe Analysis Guide for D2VP Development

## Overview
This guide provides information for future development sessions using radare2 to analyze Game.exe 1.14d retail executable for D2VP development work.

## Quick Start

### Environment Setup
```bash
cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail/
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
radare2 -w Game.exe
```

### Basic R2 Commands
```bash
# Load project
R project

# Load function renaming commands
source project/r2_commands.rc

# Analyze specific functions
pdf @ 0x0057ccb0  # UnitKilled
pdf @ 0x0057af80  # COMBAT_ApplyCold
pdf @ 0x005414d0  # GameRunUnitEvents
```

## Key Function Addresses

### Death Processing Functions
| Address | Function | Purpose | D2VP Correspondence |
|---------|---------|---------|-------------------|
| 0x0057ccb0 | sub_0057CCB0_UnitKilled | Core unit kill function | SERVER_KillMonster |
| 0x0057e990 | XP_OnMonsterKill | XP award on monster kill | SERVER_AwardKillExperience |
| 0x00543a30 | QUESTS_MonsterDeathEvent | Quest monster death event | QUEST_OnMonsterKilled |
| 0x005a6ff0 | sub_005A6FF0_MonDeathEvent | Monster death event handler | MONSTERAI_ExecuteLifecycleTransition |

### Timer Event Functions
| Address | Function | Purpose | D2VP Timer Types |
|---------|---------|---------|-----------------|
| 0x005414d0 | sub_005414D0_GameRunUnitEvents | Run unit events | General timer processing |
| 0x005417d0 | sub_005417D0_UnitQueueEvent | Queue unit events | Event queuing |
| 0x00543e10 | QUESTS_TimersRun | Timers run function | Timer execution |

### Combat State Functions
| Address | Function | Purpose | D2VP States |
|---------|---------|---------|------------|
| 0x0057af80 | COMBAT_ApplyCold | Apply cold damage/effects | STATE_cold |
| 0x0057b230 | COMBAT_ApplyFreeze | Apply freeze effects | STATE_freeze |

## D2VP Constants Reference

### Timer Event Types (from D2Headers/d2_enums.h)
```cpp
constexpr eD2TimerType UNITEVENTCALLBACK_MODECHANGE = 0;  // Animation changes
constexpr eD2TimerType UNITEVENTCALLBACK_ENDANIM = 1;     // Animation end
constexpr eD2TimerType UNITEVENTCALLBACK_AITHINK = 2;     // AI thinking
```

### Monster States (Typical Values)
- `STATE_freeze` - Completely frozen state
- `STATE_cold` - Chilled/slow state
- `STATE_uninterruptable` - Cannot be interrupted
- `STATE_death` - Death state

## Common Analysis Tasks

### 1. Analyzing Death Processing Flow
```bash
# Find UnitKilled function
s 0x0057ccb0
pdf

# Look for calls to other death functions
axf  # Find function calls
```

### 2. Timer Event Analysis
```bash
# Find timer event processing
s 0x005414d0
pdf

# Analyze event queuing
s 0x005417d0
pdf
```

### 3. State Management Analysis
```bash
# Analyze cold effects
s 0x0057af80
pdf

# Analyze freeze effects
s 0x0057b230
pdf
```

### 4. Cross-Reference with D2VP
```bash
# Look for equivalent functions in D2VP
# Use addresses from this table to find corresponding D2VP functions
# Example: 0x0057ccb0 corresponds to SERVER_KillMonster in D2VP
```

## Analysis Workflow

### Step 1: Function Identification
1. Use function addresses from this guide
2. Load corresponding R2 analysis
3. Examine function structure and purpose

### Step 2: Call Chain Analysis
1. Use `axf` to find function calls
2. Trace through call chains
3. Identify data flow and dependencies

### Step 3: State Impact Analysis
1. Look for state checks in functions
2. Identify where states block processing
3. Determine impact on timer events

### Step 4: D2VP Comparison
1. Find corresponding D2VP functions
2. Compare implementations
3. Validate fix approaches

## Troubleshooting

### R2 Environment Issues
- **Library Path**: Ensure `LD_LIBRARY_PATH` is set correctly
- **SDB Files**: Missing .sdb files are warnings only, don't prevent basic analysis
- **Project Loading**: Use `R project` to load the project configuration

### Common Errors
- **Cannot find shared libr_util.so**: Fix with LD_LIBRARY_PATH
- **Cannot find .sdb files**: These are warnings, can be ignored
- **Relocs not applied**: Use `-e bin.relocs.apply=true` or `-e bin.cache=true`

## Useful R2 Commands

### Navigation
```
s 0x0057ccb0          # Seek to address
V                     # Visual mode
pdf @ 0x0057ccb0      # Print disassembly at address
```

### Analysis
```
aaa                   # Auto-analyze
af                    # List functions
axf                   # Find function calls
```

### Project Management
```
R project             # Load project
save                  # Save project
```

## Files Structure

```
D2-Game.exe-1.14d-Retail/
├── Game.exe                  # Target executable
├── functions.json            # Function database
├── project/                  # R2 project files
│   ├── r2project.ini        # Project configuration
│   └── r2_commands.rc       # Function renaming commands
├── load_functions.py         # Python utility script
└── R2/                      # Analysis documentation
    ├── D2VP_ColdFrozen_Validation_GameExe_Analysis.md
    └── R2_Game.exe_Analysis_Guide.md  # This file
```

## Tips for Future Analysis

1. **Start with Known Addresses**: Use the function addresses table for quick reference
2. **Use Function Names**: The renamed functions make analysis easier
3. **Cross-Reference with D2VP**: Always compare findings with D2VP implementation
4. **Document Findings**: Update this guide with new findings
5. **Check for Function Calls**: Use `axf` to understand function relationships

## Contributing

When adding new findings:
1. Update this guide with new function addresses
2. Add new analysis sections as needed
3. Document any new patterns discovered
4. Update the function tables with new information

---

**Created:** 2026-11-16  
**Last Updated:** 2026-11-16  
**Version:** 1.0