# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.24)

include(GNUInstallDirs)

project(mmd-qt
    VERSION 0.2.0
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

# ---------------------------------------------------------------------------
# Desktop integration + packaging
# ---------------------------------------------------------------------------
install(FILES packaging/mmd-qt.desktop
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications)
# Every size the app ships, so a desktop that ignores the scalable icon still
# gets an exact match instead of a scaled bitmap.
foreach(_icon_size 16 24 32 48 64 128 256)
    install(FILES "resources/logo-${_icon_size}.png"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/${_icon_size}x${_icon_size}/apps
        RENAME mmd-qt.png)
endforeach()
install(FILES resources/logo.svg
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps
    RENAME mmd-qt.svg)

# The KSyntaxHighlighting library is only the engine: the language definitions
# live in a data directory it loads at runtime. Bundle that too, next to the
# usual location, so highlighting works without the distro package.
# The imported target's location property is named differently across distros
# (RELEASE / NOCONFIG / ...), so try them all, then the usual prefixes.
set(_ksyntax_candidates)
foreach(_prop IMPORTED_LOCATION_RELEASE IMPORTED_LOCATION_NOCONFIG IMPORTED_LOCATION IMPORTED_LOCATION_DEBUG)
    get_target_property(_kf6_lib KF6::SyntaxHighlighting ${_prop})
    if(_kf6_lib AND NOT _kf6_lib MATCHES "NOTFOUND")
        get_filename_component(_kf6_prefix "${_kf6_lib}/../.." ABSOLUTE)
        list(APPEND _ksyntax_candidates "${_kf6_prefix}/${CMAKE_INSTALL_DATAROOTDIR}/org.kde.syntax-highlighting")
    endif()
endforeach()
list(APPEND _ksyntax_candidates
    "/usr/${CMAKE_INSTALL_DATAROOTDIR}/org.kde.syntax-highlighting"
    "/usr/local/${CMAKE_INSTALL_DATAROOTDIR}/org.kde.syntax-highlighting")

set(_ksyntax_bundled FALSE)
foreach(_candidate IN LISTS _ksyntax_candidates)
    if(EXISTS "${_candidate}")
        install(DIRECTORY "${_candidate}/"
            DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/org.kde.syntax-highlighting")
        message(STATUS "mmd-qt: bundling syntax definitions from ${_candidate}")
        set(_ksyntax_bundled TRUE)
        break()
    endif()
endforeach()
if(NOT _ksyntax_bundled)
    message(WARNING "mmd-qt: KSyntaxHighlighting syntax definitions not found; "
        "the package will ship the engine without language definitions")
endif()

# LaTeX formulas: the vendored MicroTeX keeps its metrics compiled in, but loads
# the glyph outlines from res/fonts at run time. MathRenderer looks for the
# directory relative to the executable's data root, which is where this puts it.
install(DIRECTORY third_party/microtex/res
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/mmd-qt/microtex)

# A minimal Debian package: `cmake --build ... && cpack -G DEB`.
# Note: dpkg-shlibdeps cannot run here (the build host is not Debian-based), so
# the runtime dependencies are listed by hand. KSyntaxHighlighting is bundled
# (see src/CMakeLists.txt), so only Qt is required on the target.
set(CPACK_PACKAGE_NAME "mmd-qt")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/TankucT-AC/mmd-qt")
set(CPACK_PACKAGE_CONTACT "Magomed Gadzhiumarov <magomedgadziumarov265@gmail.com>")
set(CPACK_PACKAGE_VENDOR "mmd-qt contributors")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "editors")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS OFF)
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libqt6core6 | libqt6core6t64, libqt6gui6 | libqt6gui6t64, libqt6widgets6 | libqt6widgets6t64, libqt6svg6 | libqt6svg6t64, libqt6network6 | libqt6network6t64")
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
include(CPack)
