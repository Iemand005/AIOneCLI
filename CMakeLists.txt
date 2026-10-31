cmake_minimum_required(VERSION 3.20)
project(AIOneJS LANGUAGES C CXX)


# For metaproject
add_subdirectory(
    "../AIOne"
    "${CMAKE_CURRENT_BINARY_DIR}/AIOne"
)


# Find all source files
file(GLOB_RECURSE CORE_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Find all header files
file(GLOB_RECURSE CORE_HEADERS 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)


# Compile exe
add_executable(AIOneCLI ${CORE_SOURCES})