cmake_minimum_required(VERSION 3.22)

project(HardTune VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(HARDTUNE_BUILD_PLUGIN "Build the JUCE plugin (VST3/AU/Standalone)" ON)
option(HARDTUNE_BUILD_TESTS "Build the GoogleTest unit tests" ON)

include(FetchContent)

# Pure C++ DSP code (pitch detection, pitch shifting, scale snapping). It has no JUCE
# dependency, so it can be unit tested quickly on any platform.
add_subdirectory(dsp)

if(HARDTUNE_BUILD_PLUGIN)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    add_subdirectory(plugin)
endif()

if(HARDTUNE_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.18.0
        GIT_SHALLOW TRUE)
    # Use the same C runtime as the rest of the project on Windows
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_subdirectory(tests)
endif()
