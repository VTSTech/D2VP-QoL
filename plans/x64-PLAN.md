# D2VP-QoL x64 Migration Plan

## Current State Analysis

**Architecture**: 32-bit Windows (x86) using mingw-w64 i686 toolchain
**Build System**: CMake with hardcoded 32-bit constraints
**Compiler**: i686-w64-mingw32-gcc/g++ with MSVC x86 ABI assumptions

## Phase 1: Build System Infrastructure

### 1.1 Create mingw-x86_64.cmake Toolchain
```cmake
# Cross-compile Redivivus for 64-bit Windows with mingw-w64 (x86_64).
#
#   cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-x86_64.cmake
#
# Needs x86_64-w64-mingw32-{gcc,g++,windres,dlltool} on PATH

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

if(NOT D2_TOOL_PREFIX)
    set(D2_TOOL_PREFIX x86_64-w64-mingw32-)
endif()

set(CMAKE_C_COMPILER   ${D2_TOOL_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${D2_TOOL_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${D2_TOOL_PREFIX}gcc)
set(CMAKE_RC_COMPILER  ${D2_TOOL_PREFIX}windres)
find_program(CMAKE_DLLTOOL NAMES ${D2_TOOL_PREFIX}dlltool)

# Search the host for programs, the target sysroot for everything else
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
```

### 1.2 Update CMakeLists.txt Architecture Constraints
Modify the current hardcoded 32-bit restriction to support both architectures:
```cmake
# Replace current hardcoded 32-bit check:
# if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
#     message(FATAL_ERROR "Redivivus targets 32-bit Windows...")
# endif()

# With architecture-agnostic configuration:
if(NOT WIN32)
    message(FATAL_ERROR "Redivivus targets Windows.")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(D2_ARCH_32BIT TRUE)
    set(D2_ARCH_64BIT FALSE)
    message(STATUS "Configuring for 32-bit Windows")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(D2_ARCH_32BIT FALSE)
    set(D2_ARCH_64BIT TRUE)
    message(STATUS "Configuring for 64-bit Windows")
else()
    message(FATAL_ERROR "Unsupported pointer size: ${CMAKE_SIZEOF_VOID_P}")
endif()

# Add architecture-specific compiler flags
if(D2_ARCH_64BIT)
    add_definitions(-D_X64)
endif()
```

## Phase 2: ABI Compatibility and Code Changes

### 2.1 Critical ABI Differences to Address

**1. Pointer Sizes**
- x86: 4-byte pointers (32-bit)
- x64: 8-byte pointers (64-bit)
- **Impact**: All pointer arithmetic, structure layouts, memory management

**2. Function Call Conventions**
- x86: stdcall (stdcallcdeclcdecl for D2 specific)
- x64: Microsoft x64 ABI (rcx, rdx, r8, r9 registers for first 4 args)

**3. Structure Alignment and Padding**
- x86: 4-byte alignment typically
- x64: 8-byte alignment, different structure layouts
- **Impact**: Binary file formats, network packets, saved games

**4. Integer Sizes**
- x86: `int` is 32-bit, `long` is 32-bit
- x64: `int` is 32-bit, `long` is 64-bit
- **Impact**: All integer operations, especially in file I/O and network code

### 2.2 Code Modifications Needed

**A. Data Structure Definitions**
- Update all structure definitions for 64-bit alignment
- Add `#pragma pack(push, 8)` for network/serializable structures
- Review all `size_t` usage for pointer-width operations

**B. Function Signatures**
- Identify all D2 API callbacks that use stdcall
- Create wrapper functions for x64 ABI compatibility
- Update calling convention annotations

**C. Memory Management**
- Review all pointer arithmetic operations
- Update memory allocation patterns (64-bit addresses are larger)
- Ensure proper alignment for all memory operations

## Phase 3: Testing and Validation Strategy

### 3.1 Dual-Build Testing Matrix

| Test Case | x86 Baseline | x64 Target | Validation Method |
|-----------|-------------|------------|-------------------|
| Core Game Functions | ✓ Working | | Manual gameplay testing |
| Character Creation | ✓ Working | | Character creation/load |
| Network Multiplayer | ✓ Working | | LAN/online testing |
| File I/O (Save/Load) | ✓ Working | | Save file compatibility |
| Memory Management | ✓ Working | | Memory usage monitoring |
| Performance Metrics | ✓ Baseline | | Benchmark comparison |

### 3.2 Compatibility Testing Strategy

**A. Binary Compatibility**
- Test save game compatibility between x86 and x64 builds
- Verify network packet compatibility
- Ensure shared library interfaces work correctly

**B. Performance Benchmarking**
- Memory usage comparison (x64 typically uses more memory)
- CPU performance difference (64-bit calling convention overhead)
- Startup/load time metrics

**C. Regression Testing**
- Automated test suite for core functionality
- Comparative testing against x86 baseline
- Stress testing for memory-intensive operations

## Phase 4: Migration Implementation Roadmap

### 4.1 Immediate Tasks (Week 1-2)
- [ ] Create mingw-x86_64.cmake toolchain file
- [ ] Update CMakeLists.txt with dual-architecture support
- [ ] Initial x64 build attempt (expect failures)
- [ ] Establish test baseline from x86 build

### 4.2 Core Migration (Week 3-6)
- [ ] Fix pointer size issues throughout codebase
- [ ] Update structure definitions for 64-bit alignment
- [ ] Implement calling convention wrappers
- [ ] Address memory management changes

### 4.3 Validation and Optimization (Week 7-8)
- [ ] Complete compatibility testing
- [ ] Performance benchmarking
- [ ] Optimize 64-bit build configuration
- [ ] Document migration results

### 4.4 Release Preparation (Week 9-10)
- [ ] Final regression testing
- [ ] Build packaging for both architectures
- [ ] Update documentation
- [ ] Release dual-architecture support

## Phase 5: Risk Assessment and Mitigation

### 5.1 High-Risk Areas
- **Memory Layout Changes**: Binary incompatibility for save files
- **Network Protocol**: Potential packet size mismatches
- **Performance**: x64 may have higher memory usage

### 5.2 Mitigation Strategies
- **Versioned Save Files**: Implement save file versioning
- **Protocol Validation**: Add network packet validation
- **Memory Monitoring**: Implement comprehensive memory tracking

## Phase 6: Benefits and Expected Outcomes

### 6.1 Technical Benefits
- **Modern Hardware Support**: Access to 64-bit CPU features
- **Larger Address Space**: Potential for larger game worlds
- **Improved Performance**: Better register utilization on modern CPUs

### 6.2 User Benefits
- **Future-Proofing**: Longer-term compatibility with modern systems
- **Performance Gains**: Potential improved performance on 64-bit systems
- **Expanded Player Base**: Support for modern Windows installations

## Phase 7: Implementation Checklist

### 7.1 Build System Setup
- [ ] Create `cmake/mingw-x86_64.cmake`
- [ ] Update `CMakeLists.txt` architecture detection
- [ ] Add architecture-specific compiler flags
- [ ] Test cmake configuration for both architectures

### 7.2 Core Code Changes
- [ ] Review and update all structure definitions
- [ ] Implement proper alignment for serializable data
- [ ] Update function signatures and calling conventions
- [ ] Fix pointer arithmetic throughout codebase

### 7.3 Testing and Validation
- [ ] Establish x86 baseline metrics
- [ ] Create automated test suite
- [ ] Test save/load compatibility
- [ ] Verify network functionality
- [ ] Performance benchmarking

### 7.4 Documentation and Release
- [ ] Update build documentation
- [ ] Create migration guide
- [ ] Document any breaking changes
- [ ] Release dual-architecture builds

## Phase 8: Post-Maintenance Considerations

### 8.1 Dual Architecture Support
- Ongoing maintenance for both x86 and x64 builds
- Testing matrix expansion for new features
- Performance monitoring and optimization

### 8.2 Future Migration Paths
- Potential move to modern C++ standards
- Cross-platform considerations (Linux/macOS)
- Performance optimization strategies specific to x64

---

**Document Status**: Draft
**Last Updated**: 2026-11-16
**Target Version**: Vanilla+0.0.9+ (post-R161)
**Estimated Duration**: 10 weeks