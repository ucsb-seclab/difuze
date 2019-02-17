cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0074 OLD)
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(Utils) # For utilities

add_subdirectory(RangeAnalysis) # For Range Analysis

add_subdirectory(V4L2Processor) # v4l2 function processor.

add_subdirectory(DeviceNameFinder) # Device name finder

add_subdirectory(IoctlCmdFinder) # Smart Alias Analysis

