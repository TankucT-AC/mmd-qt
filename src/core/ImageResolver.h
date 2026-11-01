// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

/*
 * Resolves image references found in markdown to concrete file paths,
 * relative to the directory of the markdown file (as most renderers do).
 * Remote URLs are out of scope: they resolve to an empty path and the
 * renderer shows a placeholder.
 */
class ImageResolver
{
public:
    // Returns an absolute local path, or an empty string when the reference
    // cannot be resolved to a local file.
    static QString resolve(const QString &source, const QString &baseDir);

    // True when the path points to an animated image format the viewer can
    // play (GIF in v1).
    static bool isAnimated(const QString &path);
};
