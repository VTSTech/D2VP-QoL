# R2 Documentation Index

## Overview
This index provides quick access to all radare2 (R2) analysis documentation for the D2VP-QoL project. All documents are organized in the `R2/` directory for easy reference during future development and analysis sessions.

## Documentation Structure

### 📁 R2/
├── **D2VP_ColdFrozen_Validation_GameExe_Analysis.md** ← *Main Validation Report*
├── **D2VP_GameExe_Function_Mapping.md** ← *Function Cross-Reference*
├── **R2_Game.exe_Analysis_Guide.md** ← *R2 Usage Guide*
├── **R2_Project_Setup_Guide.md** ← *Project Setup Instructions*
├── **ColdFrozen_Bug_Complete_Analysis.md** ← *Bug Investigation Record*
└── **README.md** ← *This Index File*

## Quick Start Guide

### For New Users
1. **Read Setup Guide**: `R2_Project_Setup_Guide.md`
2. **Learn R2 Basics**: `R2_Game.exe_Analysis_Guide.md`
3. **View Function Mapping**: `D2VP_GameExe_Function_Mapping.md`
4. **Understand the Fix**: `D2VP_ColdFrozen_Validation_GameExe_Analysis.md`

### For Returning Users
- **Quick Reference**: Use this README for document links
- **Bug Analysis**: `ColdFrozen_Bug_Complete_Analysis.md`
- **Function Lookup**: `D2VP_GameExe_Function_Mapping.md`

## Document Descriptions

### 🔬 Analysis Reports

#### 1. **D2VP_ColdFrozen_Validation_GameExe_Analysis.md**
**Purpose**: Main validation report for the cold/frozen enemies fix
**Contents**: 
- Complete Game.exe analysis results
- Function address mapping
- Validation confirmation
- Technical recommendations
**For**: Developers needing validation confirmation

#### 2. **D2VP_GameExe_Function_Mapping.md**
**Purpose**: Cross-reference D2VP functions with Game.exe equivalents
**Contents**:
- Detailed function mapping tables
- Implementation comparisons
- Data structure mapping
- Critical patterns identified
**For**: Developers working on D2VP features

#### 3. **ColdFrozen_Bug_Complete_Analysis.md**
**Purpose**: Complete bug investigation and resolution record
**Contents**:
- Issue timeline and symptoms
- Root cause analysis
- Fix evolution (R161 → R162)
- Testing and validation results
**For**: Understanding the complete bug history

### 🛠️ Operational Guides

#### 4. **R2_Game.exe_Analysis_Guide.md**
**Purpose**: Quick reference guide for R2 analysis work
**Contents**:
- Key function addresses
- Common analysis commands
- Analysis workflow
- Troubleshooting tips
**For**: Daily analysis sessions

#### 5. **R2_Project_Setup_Guide.md**
**Purpose**: Complete setup and configuration guide
**Contents**:
- Installation requirements
- Environment setup
- Project configuration
- Performance optimization
**For**: New installations or troubleshooting

## Quick Reference Tables

### Key Function Addresses
| Function Type | Address | Purpose |
|---------------|---------|---------|
| UnitKilled | 0x0057ccb0 | Core death processing |
| XP_OnMonsterKill | 0x0057e990 | XP award processing |
| QUESTS_MonsterDeathEvent | 0x00543a30 | Quest death events |
| GameRunUnitEvents | 0x005414d0 | Event processing |
| UnitQueueEvent | 0x005417d0 | Event queuing |

### Critical Timer Events
| Timer Type | Value | Purpose |
|-----------|-------|---------|
| UNITEVENTCALLBACK_MODECHANGE | 0 | Animation changes |
| UNITEVENTCALLBACK_ENDANIM | 1 | Animation end |
| UNITEVENTCALLBACK_AITHINK | 2 | AI thinking |

### Environment Setup Commands
```bash
cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail/
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
radare2 -w Game.exe
R project
source project/r2_commands.rc
```

## Common Tasks

### 1. Analyzing Death Processing
**Documents**: 
- Main validation report
- Function mapping
- Bug analysis

**Commands**:
```bash
s 0x0057ccb0
pdf
```

### 2. Understanding Timer Events
**Documents**:
- Function mapping
- Analysis guide

**Commands**:
```bash
s 0x005414d0
pdf
```

### 3. Setting Up R2 Environment
**Documents**:
- Setup guide
- Analysis guide

**Commands**:
```bash
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
```

### 4. Validating D2VP Fix
**Documents**:
- Main validation report
- Bug analysis

**Commands**:
```bash
# Check bypass implementation
grep -r "if (false)" Source/
```

## Troubleshooting

### Common Issues
- **Library problems**: See setup guide for LD_LIBRARY_PATH
- **Function loading**: Check r2_commands.rc syntax
- **Analysis errors**: Use analysis guide for debugging

### Quick Commands
```bash
# Check project loaded
R project

# List functions
af

# Check specific function
af @ 0x0057ccb0
```

## File Dependencies

### Required Files
- `D2-Game.exe-1.14d-Retail/Game.exe` - Target executable
- `D2-Game.exe-1.14d-Retail/functions.json` - Function database
- `D2-Game.exe-1.14d-Retail/project/r2_commands.rc` - Function renaming

### Optional Files
- Python script for regenerating commands
- Backup files for disaster recovery

## Contributing to Documentation

### When Adding New Documents
1. Place new files in the `R2/` directory
2. Add description to this README
3. Include reference tables where applicable
4. Update cross-references between documents

### Document Standards
- Use consistent naming: `Topic_Description.md`
- Include metadata section with dates
- Provide clear purpose and contents descriptions
- Include quick reference tables for technical content

## Search and Navigation

### Finding Specific Information
1. **By Topic**: Use document descriptions in this README
2. **By Function**: Use function mapping table
3. **By Issue**: Use bug analysis document

### File Locations
- **Root**: `/home/vtstech/workspace/D2VP-QoL/`
- **R2 Directory**: `/home/vtstech/workspace/D2VP-QoL/R2/`
- **Game.exe**: `/home/vtstech/workspace/D2-Game.exe-1.14d-Retail/`

## Update Log

### Version History
- **v1.0** (2026-11-16): Initial documentation set
  - Main validation report
  - Function mapping
  - R2 usage guide
  - Project setup guide
  - Bug analysis record
  - This index file

### Future Updates
- Add new function mappings as discovered
- Update validation results for new fixes
- Expand troubleshooting section with new issues
- Add examples of common analysis patterns

## Getting Started

### New Users
1. **Read this README** to understand documentation structure
2. **Follow setup guide** to configure R2 environment
3. **Review validation report** to understand the main fix
4. **Use function mapping** for D2VP development

### Returning Users
1. **Use this README** as quick navigation guide
2. **Reference specific documents** as needed
3. **Update documentation** with new findings
4. **Follow established patterns** for new analyses

---

**Created**: 2026-11-16  
**Last Updated**: 2026-11-16  
**Version**: 1.0  
**Total Documents**: 6