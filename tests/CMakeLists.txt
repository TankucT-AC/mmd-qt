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

add_executable(test_fonts
    test_fonts.cpp
)
target_link_libraries(test_fonts PRIVATE mmd_core Qt6::Test)

add_test(NAME test_fonts COMMAND test_fonts)
set_tests_properties(test_fonts PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_markdown
    test_markdown.cpp
)
target_link_libraries(test_markdown PRIVATE mmd_core Qt6::Test)

add_test(NAME test_markdown COMMAND test_markdown)
set_tests_properties(test_markdown PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_editor
    test_editor.cpp
)
target_link_libraries(test_editor PRIVATE mmd_core Qt6::Test)

add_test(NAME test_editor COMMAND test_editor)
set_tests_properties(test_editor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(test_view
    test_view.cpp
)
target_link_libraries(test_view PRIVATE mmd_core Qt6::Test)

add_test(NAME test_view COMMAND test_view)
set_tests_properties(test_view PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
