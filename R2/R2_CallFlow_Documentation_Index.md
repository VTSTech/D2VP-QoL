# R2 Call Flow Documentation Index

## Overview
Comprehensive call flow documentation for common Diablo II gameplay scenarios, providing complete analysis of function calls, data structures, and integration points for D2VP development.

## Documentation Structure

### 📁 R2/
```
R2/
├── R2_Game.exe_Validation_Expansion_R170.md   # capstone re-verification + monster timer dispatch
├── R2_Game.exe_Diagnostic_Function_Reference_R171.md  # symptom -> function diagnostic reference (START HERE when debugging)
├── R2_Game.exe_Analysis_Guide.md              # R2 usage guide
├── R2_Project_Setup_Guide.md                  # Setup instructions
├── ColdFrozen_DeathPath_r2_Verified_R170.md   # cold/frozen death path, binary-verified
├── ColdFrozen_Bug_Complete_Analysis.md        # cold/frozen bug history and fix status
├── D2VP_ColdFrozen_Validation_GameExe_Analysis.md  # R162-R164 validation vs retail
├── D2VP_GameExe_Function_Mapping.md           # D2VP <-> Game.exe function mapping
├── Killing_Monster_CallFlow.md                # Monster death and XP (addresses re-verified R170b)
├── Battle.net_Authentication_Flow_Analysis.md # BN packet-level flow (constants only)
├── CD_Key_Functions_Game.exe_Analysis.md      # CD key handling
├── CD_Key_Analysis_Report.md                  # CD key analysis
├── CheckRevision_Analysis_and_Fix_Strategies.md # CheckRevision / CRC strategies
└── R2_CallFlow_Documentation_Index.md          # this index
```

## Quick Reference Tables

### Scenario Coverage

| Scenario | File | Priority | Complexity |
|----------|------|----------|------------|
| Killing a Monster | `Killing_Monster_CallFlow.md` | High | High |
| Cold/Frozen Monster Death (issue #39) | `ColdFrozen_DeathPath_r2_Verified_R170.md` | High | High |
| Monster Timer Dispatch (regen/poison/AI) | `R2_Game.exe_Validation_Expansion_R170.md` | High | High |
| Symptom-driven debugging (any field report) | `R2_Game.exe_Diagnostic_Function_Reference_R171.md` | High | — |

> **R170b housekeeping:** the Boot/Entering/Loot/Chest/Shrine and TCP/IP + Battle.net
> call-flow docs were **removed** — their address tables were placeholders or 1.10-1.13-era
> DLL addresses (verified against the 1.14d section map; every one falls in `.rdata`, i.e.
> not code). Rebuild them only from re-derived pins. See
> `R2_Game.exe_Validation_Expansion_R170.md` §4 for the audit.

### Key Addresses Reference

| Component | Verified Address | Purpose |
|-----------|------------------|---------|
| SetUninterruptableState | 0x005544b0 | clears uninterruptable AND completes a deferred death |
| SERVER_KillMonster | 0x0057ccb0 | core death processing (16 xrefs) |
| SERVER_GetHitCalculation | 0x0057cee0 | damage/kill application; deferral site 1 @ 0x57cf7b |
| RegenAndPsnDecreaser | 0x005a6920 | hp-regen/poison tick; deferral site 2 @ 0x5a6ac5 |
| MONSTEREVENT_DefaultCallback | 0x005a7f80 | monster timer dispatcher (freeze gate + route table) |
| Monster pTimerMethods table | 0x006e2490 | 15-slot function table (nulls 4/11/13/14) |
| UNITEVENT_DispatchEventsByType | 0x005c0c30 | after-kill event dispatch (types 9/10) |
| XP award | 0x0057e990 | via KillMonster wrapper 0x57cd44 -> 0x5a4ef0 |
| Quest monster-death event | 0x00543a30 | via same wrapper -> 0x57cdb7 |
| COMBAT_ApplyCold / ApplyFreeze | 0x0057af80 / 0x0057b230 | cold/freeze state application |
| Core Death | 0x0057ccb0 | Unit death processing |
| XP Award | 0x0057e990 | XP calculation |
| Quest Events | 0x00543a30 | Quest triggers |
| AI Death | 0x005a6ff0 | Monster death AI |

### Function Categories

#### Boot & Entry
- Entry point initialization
- Window creation and D2Win setup
- Character selection
- Login and networking
- Game loading and map generation
- Player spawn

#### Combat & Death
- Damage calculation
- Death processing
- AI lifecycle transitions
- XP award calculation
- Quest kill events
- Death animation

#### Loot & Items
- Loot drop probability
- Item generation and stat generation
- Item placement on ground
- Chest interaction and loot
- Shrine activation and effects
- Item pickup and stacking

#### Object Interaction
- Object hit detection
- Interaction animations
- State management
- Visual effects
- Particle systems

## Usage Guide

### For New Users

1. **Read Setup Guide First**
   - `R2_Project_Setup_Guide.md` - Environment setup
   - `R2_Game.exe_Analysis_Guide.md` - R2 usage basics

2. **Understand Scenario**
   - Read the scenario call flow document
   - Understand the complete sequence
   - Identify key data structures

3. **Analyze in R2**
   ```bash
   cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail/
   export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
   export R2_USER_BASE="/tmp/r2-data"
   r2 -w Game.exe
   R project
   source project/r2_commands.rc
   ```

4. **Locate Functions**
   - Use function names from documentation
   - Find addresses in table
   - Analyze with `pdf` command

### For Experienced Users

1. **Quick Reference**
   - Use address table for rapid navigation
   - Focus on function signatures and data structures

2. **Deep Dive**
   - Analyze complete function call chains
   - Trace data flow through structures
   - Identify integration points

3. **Modification Planning**
   - Review D2VP integration points
   - Identify what needs to be overridden
   - Plan modifications for custom behavior

### For Debugging

1. **When Something Doesn't Work**
   - Check the specific function calls
   - Verify state transitions
   - Check data structure contents

2. **When Adding New Features**
   - Use call flows as template
   - Identify insertion points
   - Plan complete sequence

3. **When Investigating Bugs**
   - Trace complete execution path
   - Check all state changes
   - Verify data integrity

## Critical Insights

### Common Patterns

1. **State Machine Flow**
   - All objects use state machines
   - State changes trigger specific events
   - Completion callbacks trigger next steps

2. **Animation-Driven Processing**
   - Many processes wait for animations
   - Completion triggers next step
   - Visual feedback is critical

3. **Data Flow Patterns**
   - Packet → Parse → Validate → Process → Store → Display
   - Data structures flow through system
   - Validation at each step

4. **Interaction Sequence**
   - Hit detection → Validation → Animation → Processing → Completion
   - Validation gates prevent bad interactions
   - Animation provides feedback

### Integration Points

#### What D2VP Overrides
- All initialization sequences
- Network packet handling
- State machine transitions
- Animation timing and effects
- Visual rendering
- UI updates

#### What D2VP Enhances
- Custom items and loot
- Custom game mechanics
- Modified damage formulas
- Enhanced UI/UX
- Custom game modes

### Performance Considerations

1. **Memory Allocation**
   - Track allocations during flow
   - Identify memory leaks
   - Optimize frequently allocated structures

2. **CPU Usage**
   - Animation loops consume CPU
   - Particle systems expensive
   - Network processing can lag

3. **Network Overhead**
   - Packet serialization cost
   - Broadcast frequency
   - Response time requirements

## Related Documentation

### R2 Analysis Documentation
- `D2VP_GameExe_Function_Mapping.md` - Function cross-reference
- `D2VP_ColdFrozen_Validation_GameExe_Analysis.md` - Death processing validation
- `ColdFrozen_Bug_Complete_Analysis.md` - Bug investigation record (status reopened R170, residual strand H1)
- `ColdFrozen_DeathPath_r2_Verified_R170.md` - r2-verified retail address map + residual bug analysis (R170)

### Project Memories
- `shrine-parameter-typing-fix-r167-complete.md` - Shrine fix documentation
- `x64-migration-plan.md` - Architecture migration strategy

### Code Analysis
- D2VP source code structure
- Function call graphs
- Data structure definitions

## Common Tasks

### Finding a Function
1. Read the call flow document
2. Find the function name in table
3. Locate address in address table
4. Use `s address; pdf` to view function
5. Use `axf` to find call tree

### Tracing Data Flow
1. Identify start point
2. Follow function calls
3. Track structure contents
4. Identify where data changes
5. Note final destination

### Finding Integration Points
1. Read scenario documentation
2. Identify override points
3. Review D2VP integration notes
4. Check existing D2VP code
5. Plan modifications

### Analyzing Performance
1. Identify expensive operations
2. Check loop frequencies
3. Analyze memory allocations
4. Review network usage
5. Plan optimizations

## Quick Commands

### Navigation
```bash
# Navigate to address
s 0x0057ccb0

# Print disassembly
pdf

# Print function list
af

# Find all functions matching string
iz~string

# Find call tree
axf @ address

# List references
axt @ address
```

### Analysis
```bash
# Visual mode
V

# Graph mode
agf

# String search
izz

# Symbol search
iz~

# Find xrefs
axt
```

### Project Management
```bash
# Load project
R project

# Source commands
source project/r2_commands.rc

# Save analysis
save

# Save project
qy
```

## File Dependencies

### Required for Analysis
- `Game.exe` - Target executable (3.6MB)
- `functions.json` - Function database (689KB)
- `project/r2_commands.rc` - Function renaming (3,900+ lines)
- `R2/*.md` - Call flow documentation

### Optional for Analysis
- `load_functions.py` - Regenerate function commands
- Python scripts - Custom analysis tools

## Contributing Documentation

### When Adding New Documentation
1. Choose appropriate scenario
2. Use consistent naming: `Topic_CallFlow.md`
3. Include complete call flow
4. Add address tables
5. Include D2VP integration points
6. Add analysis commands

### Documentation Standards
- Use ASCII arrows for call flow
- Include complete function signatures
- Document all data structures
- List all key addresses
- Include D2VP integration notes
- Provide R2 analysis commands

## Troubleshooting

### Common Issues

1. **Cannot Find Function**
   - Check address table
   - Verify R2 project loaded
   - Check function mapping

2. **Analysis Errors**
   - Check R2 commands
   - Verify function names
   - Check library paths

3. **Memory Issues**
   - Clear R2 cache
   - Reduce analysis depth
   - Use cached analysis

## Future Enhancements

### Planned Additions
- [ ] Item identification flow
- [ ] Spell casting flow
- [ ] NPC dialogue flow
- [ ] Quest completion flow
- [ ] Item upgrading flow
- [ ] Socketing flow

### Potential Improvements
- Add call graphs
- Include code examples
- Add test scenarios
- Include performance metrics
- Add optimization tips

## Version History

- **v1.0** (2026-11-17): Initial documentation set
  - 6 comprehensive call flow documents
  - Complete scenario coverage
  - Full integration points
  - R2 analysis commands

---

**Created**: 2026-11-17
**Last Updated**: 2026-11-17
**Version**: 1.0
**Total Documents**: 6 call flows + 3 guides
**Coverage**: 100% of requested scenarios
