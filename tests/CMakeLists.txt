# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Every test links the real production library and runs headless, so the suite
# behaves the same on a developer machine and in CI. Extra arguments after the
# name are passed on as compile definitions.
function(mmd_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE mmd_core Qt6::Test)
    if(ARGC GREATER 1)
        target_compile_definitions(${name} PRIVATE ${ARGN})
    endif()
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endfunction()

mmd_add_test(test_app)
mmd_add_test(test_theme)
mmd_add_test(test_fonts)
mmd_add_test(test_markdown MMD_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
mmd_add_test(test_editor)
mmd_add_test(test_view)
mmd_add_test(test_html)
mmd_add_test(test_code)
mmd_add_test(test_gif)
mmd_add_test(test_file)
mmd_add_test(test_limits)
mmd_add_test(test_find)
mmd_add_test(test_outline)
mmd_add_test(test_sidebar)
mmd_add_test(test_math)
mmd_add_test(test_imagefetcher)
