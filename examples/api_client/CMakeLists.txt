# ============================================================
# HLV-RAPS REST API Demo
# ============================================================

cmake_minimum_required(VERSION 3.16)

project(raps_rest_api_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------
# Demo executable
# ------------------------------------------------------------

add_executable(rest_api_demo
    rest_api_demo.cpp
    ../../src/api/rest_api_server.cpp
    ../../src/platform/platform_hal.cpp
)

# ------------------------------------------------------------
# Include paths
# ------------------------------------------------------------

target_include_directories(rest_api_demo PRIVATE
    ${PROJECT_SOURCE_DIR}/../../include
    ${PROJECT_SOURCE_DIR}/../../src
)

# ------------------------------------------------------------
# Threading support
# ------------------------------------------------------------

find_package(Threads REQUIRED)
target_link_libraries(rest_api_demo PRIVATE Threads::Threads)

# ------------------------------------------------------------
# Warnings & safety flags
# ------------------------------------------------------------

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(rest_api_demo PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
endif()
