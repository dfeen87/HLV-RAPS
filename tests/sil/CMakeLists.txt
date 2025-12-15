# ============================================================
# RAPS — Software-in-the-Loop (SIL) Test Suite
# ============================================================

cmake_minimum_required(VERSION 3.16)

project(raps_sil_tests LANGUAGES CXX)

# ------------------------------------------------------------
# Build options
# ------------------------------------------------------------

option(RAPS_ENABLE_SIL_FAULTS
    "Enable SIL fault injection in PlatformHAL"
    ON
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ------------------------------------------------------------
# Test executable
# ------------------------------------------------------------

add_executable(raps_sil_fault_tests
    test_fault_injection.cpp
)

# ------------------------------------------------------------
# Include paths
# ------------------------------------------------------------

target_include_directories(raps_sil_fault_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

# ------------------------------------------------------------
# Compile definitions
# ------------------------------------------------------------

if (RAPS_ENABLE_SIL_FAULTS)
    target_compile_definitions(raps_sil_fault_tests PRIVATE
        RAPS_ENABLE_SIL_FAULTS=1
    )
endif()

# ------------------------------------------------------------
# Warnings & safety flags
# ------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(raps_sil_fault_tests PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wshadow
        -Wconversion
        -Wno-unused-parameter
    )
endif()

# ------------------------------------------------------------
# Optional coverage instrumentation
# (enabled later when we add coverage gates)
# ------------------------------------------------------------

option(RAPS_ENABLE_COVERAGE "Enable coverage instrumentation" OFF)

if (RAPS_ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(STATUS "SIL coverage instrumentation enabled")

    target_compile_options(raps_sil_fault_tests PRIVATE
        -O0
        -g
        --coverage
    )

    target_link_options(raps_sil_fault_tests PRIVATE
        --coverage
    )
endif()

# ------------------------------------------------------------
# Register with CTest
# ------------------------------------------------------------

enable_testing()

add_test(
    NAME raps_sil_fault_injection
    COMMAND raps_sil_fault_tests
)
