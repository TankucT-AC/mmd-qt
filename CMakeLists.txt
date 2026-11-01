# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

project(mmd-qt
    VERSION 0.1.0
    DESCRIPTION "Two-pane Markdown viewer and editor built with Qt Widgets"
    LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Widgets Test Svg)
find_package(KF6SyntaxHighlighting 6.0 REQUIRED)

# Strict warnings for this project's own translation units. They are attached as
# an INTERFACE target and linked PUBLIC, so tests and the executable inherit
# them; Qt/KF6 headers stay system headers and are not affected.
# -ansi is intentionally omitted: it selects C++98 and would break C++20.
add_library(mmd_warnings INTERFACE)
target_compile_options(mmd_warnings INTERFACE
    $<$<CXX_COMPILER_ID:GNU,Clang>:
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wpointer-arith
        -Wcast-qual -Wcast-align -Wwrite-strings -Wfloat-equal
        -Woverloaded-virtual -Wundef -Wuninitialized -Wredundant-decls
        -Wnon-virtual-dtor -Wctor-dtor-privacy -Wzero-as-null-pointer-constant
        -Wsign-promo -Wold-style-cast
    >
    $<$<CXX_COMPILER_ID:GNU>:
        -fno-common -Wuseless-cast -Wsuggest-override -Wlogical-op
        -Wduplicated-cond -Wduplicated-branches -Wnoexcept
    >
)

add_subdirectory(third_party)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
