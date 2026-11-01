# SPDX-FileCopyrightText: 2026 mmd-qt contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Vendored third-party code. Every directory keeps the upstream license file
# verbatim; do not strip license headers when updating these sources.

# md4c: Markdown parser (MIT). Upstream: https://github.com/mity/md4c
add_library(md4c STATIC
    md4c/src/md4c.c
    md4c/src/entity.c
)
target_include_directories(md4c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/md4c/src)
