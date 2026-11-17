# R2 Call Flow Documentation Index

## Overview
Comprehensive call flow documentation for common Diablo II gameplay scenarios, providing complete analysis of function calls, data structures, and integration points for D2VP development.

## Documentation Structure

### 📁 R2/
```
R2/
├── Boot_TitleScreen_CallFlow.md              # Title screen to game entry
├── Entering_Game_CallFlow.md                  # Character selection to game
├── Killing_Monster_CallFlow.md                # Monster death and XP
├── Monster_Dropping_Loot_CallFlow.md          # Item generation and pickup
├── Opening_Chest_CallFlow.md                  # Chest interaction and loot
├── Activating_Shrine_CallFlow.md              # Shrine activation and effects
├── R2_Game.exe_Analysis_Guide.md              # R2 usage guide
├── R2_Project_Setup_Guide.md                  # Setup instructions
└── README.md                                   # Documentation index
```

## Quick Reference Tables

### Scenario Coverage

| Scenario | File | Priority | Complexity |
|----------|------|----------|------------|
| Boot to Title Screen | `Boot_TitleScreen_CallFlow.md` | High | Medium |
| Entering a Game | `Entering_Game_CallFlow.md` | High | High |
| Killing a Monster | `Killing_Monster_CallFlow.md` | High | High |
| Monster Dropping Loot | `Monster_Dropping_Loot_CallFlow.md` | Medium | Medium |
| Opening a Chest | `Opening_Chest_CallFlow.md` | Medium | Medium |
| Activating a Shrine | `Activating_Shrine_CallFlow.md` | Medium | Low |

### Key Addresses Reference

| Component | Typical Address | Purpose |
|-----------|----------------|---------|
| Entry Point | 0x00401000 | Game.exe initialization |
| Title Screen | 0x00406000 | Title screen rendering |
| Character Select | 0x00407000 | Character selection |
| Game Entry | 0x00408000 | Game connection |
| Map Generation | 0x00410000 | Level generation |
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
- `ColdFrozen_Bug_Complete_Analysis.md` - Bug investigation record

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
