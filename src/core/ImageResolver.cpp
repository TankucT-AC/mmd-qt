// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/ImageResolver.h"

#include <QDir>
#include <QFileInfo>
#include <QUrl>

QString ImageResolver::resolve(const QString &source, const QString &baseDir)
{
    if (source.isEmpty())
        return QString();

    const QUrl url(source);
    if (url.isRelative()) {
        if (baseDir.isEmpty())
            return QFileInfo(source).absoluteFilePath();
        return QDir(baseDir).filePath(source);
    }

    if (url.isLocalFile())
        return url.toLocalFile();

    // Remote and exotic schemes are not supported in v1.
    return QString();
}

bool ImageResolver::isAnimated(const QString &path)
{
    static const QStringList suffixes = {QStringLiteral("gif")};
    return suffixes.contains(QFileInfo(path).suffix(), Qt::CaseInsensitive);
}
