cmake_minimum_required(VERSION 3.13)
project(LLVM_Assignment_1 LANGUAGES C CXX)

# Tell CMake where LLVM 22 is installed
set(LLVM_DIR "/usr/local/lib/cmake/llvm")  # adjust if different

# Find LLVM package
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM dir = ${LLVM_DIR}")

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include LLVM helper macros
include(AddLLVM)

# Add the src (passes) subdirectory
add_subdirectory(src)
