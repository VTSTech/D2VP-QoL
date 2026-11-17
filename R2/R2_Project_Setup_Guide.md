# R2 Project Setup and Usage Guide

## Overview
This guide provides detailed instructions for setting up and using the radare2 (R2) project for analyzing Game.exe 1.14d retail executable. This setup enables binary analysis to understand Diablo II's internal workings and validate D2VP fixes.

## Prerequisites
- Linux environment with Python 3
- radare2 6.1.5 portable version installed to /tmp/r2-portable
- Functions database (functions.json) for Game.exe 1.14d

## Quick Setup

### 1. Environment Variables
```bash
cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail/
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"
```

### 2. Launch R2
```bash
radare2 -w Game.exe
```

### 3. Load Project
```bash
R project
source project/r2_commands.rc
```

## Detailed Setup Instructions

### Step 1: Verify Installation
```bash
# Check if radare2 is installed
which radare2
# Expected: /tmp/r2-portable/bin/radare2

# Check version
radare2 -v
# Expected: 6.1.5
```

### Step 2: Directory Structure
```
D2-Game.exe-1.14d-Retail/
├── Game.exe                  # Target executable (3.6MB)
├── functions.json            # Function database (689KB)
├── project/                  # R2 project directory
│   ├── r2project.ini        # Project configuration
│   └── r2_commands.rc       # Function renaming commands (3,900+ lines)
├── load_functions.py         # Python script for processing
└── R2/                      # Analysis documentation
```

### Step 3: Load Project Configuration
```bash
# Load the project
R project

# List project info
i

# Load function names
source project/r2_commands.rc
```

### Step 4: Verify Function Loading
```bash
# List first few renamed functions
af | head -10

# Look for specific functions
af @ 0x0057ccb0
```

## Common Analysis Commands

### Basic Navigation
```bash
# Seek to address
s 0x0057ccb0

# Print disassembly
pdf

# Visual mode
V

# Go to entry point
s entry
```

### Function Analysis
```bash
# List all functions
af

# Analyze specific function
pdf @ 0x0057ccb0

# Find function calls
axf

# List function graphs
agf
```

### Data Analysis
```bash
# List symbols
is

# List imports
ii

# List exports
ie

# Analyze data sections
idata
```

### Project Management
```bash
# Save project
save

# Exit project
q

# Reload project
R project
```

## Environment Troubleshooting

### Common Issues and Solutions

#### 1. Library Path Issues
**Problem:** `Cannot find shared libr_util.so`
```bash
# Fix: Set LD_LIBRARY_PATH
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
```

#### 2. SDB File Warnings
**Problem:** `Cannot find /home/z/.local/share/radare2/6.1.5/format/dll/ws2_32.sdb`
```bash
# Note: These are warnings only, don't prevent analysis
# Use temporary directory to suppress warnings
export R2_USER_BASE="/tmp/r2-data"
```

#### 3. Relocation Warnings
**Problem:** `Relocs has not been applied`
```bash
# Fix: Enable relocation cache
radare2 -w Game.exe -e bin.relocs.apply=true
```

### Full Environment Setup Script
```bash
#!/bin/bash
# setup_r2_environment.sh

cd /home/vtstech/workspace/D2-Game.exe-1.14d-Retail/

# Set environment variables
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"

# Launch radare2
radare2 -w Game.exe
```

## Function Renaming System

### How Renaming Works
The system processes functions.json and creates R2 commands to rename functions:
- `sub_XXXXXXXX_FunctionName` → `D2_sub_XXXXXXXX_FunctionName`
- API functions kept as-is (e.g., `GetCurrentThreadId`)

### Processing Script
```python
# load_functions.py
def load_functions_to_radare2():
    with open('functions.json', 'r') as f:
        data = json.load(f)
    
    with open('project/r2_commands.rc', 'w') as f:
        f.write('# Radare2 commands to load function names\n')
        f.write('aaa\n')
        f.write('aeim\n')
        f.write('af\n')
        
        for func in data['functions']:
            address = func['address']
            old_name = func['name']
            
            if func.get('is_thunk', False):
                continue
                
            if old_name.startswith('sub_'):
                new_name = f"D2_{old_name}"
            else:
                new_name = old_name
            
            f.write(f'afn {new_name} {address}\n')
```

### Regenerating Commands
If functions.json updates, regenerate the commands:
```bash
python3 load_functions.py
```

## Analysis Workflow

### 1. Initial Analysis
```bash
# Auto-analyze the binary
aaa

# Load project
R project

# Load function names
source project/r2_commands.rc

# Get overview
i
```

### 2. Function Analysis
```bash
# Analyze specific function
s 0x0057ccb0
pdf

# Get function info
af @ 0x0057ccb0

# Find cross-references
axf @ 0x0057ccb0
```

### 3. Data Analysis
```bash
# Analyze data sections
idata

# Find strings
izz

# Find references
isz
```

### 4. Export Results
```bash
# Save analysis results
pdf @ 0x0057ccb0 > unitkilled_analysis.txt

# Export function list
af > function_list.txt
```

## Performance Optimization

### Speed Up Analysis
```bash
# Use cached analysis
radare2 -w Game.exe -e bin.cache=true

# Disable unnecessary features
radare2 -w Game.exe -e anal.hasret=false
```

### Memory Management
```bash
# Clear cache when needed
rm -rf /tmp/r2-data/*

# Monitor memory usage
ps aux | grep radare2
```

## Integration with D2VP Development

### Cross-Reference Workflow
1. **Identify Issue**: Find D2VP bug or question
2. **Find Corresponding Function**: Use function mapping to locate Game.exe equivalent
3. **Analyze Game.exe**: Use R2 to analyze function behavior
4. **Compare Implementations**: Validate D2VP approach against Game.exe
5. **Document Findings**: Update analysis documentation

### Example Analysis Session
```bash
# Setup environment
export LD_LIBRARY_PATH="/tmp/r2-portable/lib:$LD_LIBRARY_PATH"
export R2_USER_BASE="/tmp/r2-data"

# Launch R2
radare2 -w Game.exe

# In R2:
R project
source project/r2_commands.rc
s 0x0057ccb0
pdf
axf
```

## Backup and Maintenance

### Regular Maintenance
```bash
# Backup project files
cp -r project project_backup_$(date +%Y%m%d)

# Verify function loading
af | wc -l
# Should show ~3,900 functions

# Check for errors
radare2 -w Game.exe -q -c "R project; af" | grep -i error
```

### Update Process
```bash
# If functions.json updates:
python3 load_functions.py
source project/r2_commands.rc
```

## Security Considerations

### File Permissions
- Ensure Game.exe is readable by your user
- Keep radare2 installation secure
- Don't execute Game.exe in the analysis environment

### Data Sensitivity
- Game.exe contains proprietary Blizzard code
- Only use for analysis purposes
- Don't redistribute or modify the executable

## Getting Help

### Resources
- R2 documentation: `man radare2`
- R2 wiki: https://github.com/radareorg/radare2/wiki
- This guide: `/home/vtstech/workspace/D2VP-QoL/R2/`

### Common Commands Help
```bash
# R2 help
?

# Command specific help
pdf?
af?
```

---

**Setup Date:** 2026-11-16  
**Last Updated:** 2026-11-16  
**Version:** 1.0