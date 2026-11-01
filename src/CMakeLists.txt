# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

qt_standard_project_setup()

# Internal static library: everything except main(). Tests link against it,
# so one binary under test exercises the real production code.
add_library(mmd_core STATIC
    ../third_party/fonts/fonts.qrc
    app/MainWindow.h
    app/MainWindow.cpp
    core/Theme.h
    core/Theme.cpp
    core/ThemeManager.h
    core/ThemeManager.cpp
    core/FontManager.h
    core/FontManager.cpp
)

target_include_directories(mmd_core PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(mmd_core
    PUBLIC
        Qt6::Widgets
        KF6::SyntaxHighlighting
        md4c
)
add_executable(mmd
    main.cpp
)
target_link_libraries(mmd PRIVATE mmd_core)
