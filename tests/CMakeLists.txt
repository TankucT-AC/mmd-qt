# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(test_app
    test_app.cpp
)
target_link_libraries(test_app PRIVATE mmd_core Qt6::Test)

add_test(NAME test_app COMMAND test_app)
set_tests_properties(test_app PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_theme
    test_theme.cpp
)
target_link_libraries(test_theme PRIVATE mmd_core Qt6::Test)

add_test(NAME test_theme COMMAND test_theme)
set_tests_properties(test_theme PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
