cmake_minimum_required(VERSION 3.20)
project(AIOneCLI LANGUAGES C CXX)

# To speed up compilation for hwinfo cuz we only need CPU core info
set(HWINFO_COMPUTER OFF CACHE BOOL "" FORCE)
set(HWINFO_MAINBOARD OFF CACHE BOOL "" FORCE)
set(HWINFO_DISK OFF CACHE BOOL "" FORCE)
set(HWINFO_RAM OFF CACHE BOOL "" FORCE)
set(HWINFO_GPU OFF CACHE BOOL "" FORCE)
set(HWINFO_GPU_BACKENDS OFF CACHE BOOL "" FORCE)
set(HWINFO_BATTERY OFF CACHE BOOL "" FORCE)
set(HWINFO_NETWORK OFF CACHE BOOL "" FORCE)


# For metaproject
set(GIT_EXECUTABLE "GIT_EXECUTABLE-NOTFOUND" CACHE FILEPATH "Disable Git tracking in dependencies" FORCE)
add_subdirectory("../AIOne" "${CMAKE_CURRENT_BINARY_DIR}/AIOne")
add_subdirectory("external/argparse" "${CMAKE_CURRENT_BINARY_DIR}/external_argparse")
add_subdirectory("external/hwinfo" "${CMAKE_CURRENT_BINARY_DIR}/external_hwinfo")


# Find all source files
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Find all header files
file(GLOB_RECURSE CORE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# Compile exe
add_executable(AIOneCLI ${CORE_SOURCES})

# Include header files
target_include_directories(AIOneCLI PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(AIOneCLI PRIVATE AIOneCore) 
target_link_libraries(AIOneCLI PRIVATE argparse::argparse)
target_link_libraries(AIOneCLI PRIVATE lfreist-hwinfo::hwinfo)