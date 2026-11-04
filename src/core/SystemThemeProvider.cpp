// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/SystemThemeProvider.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QStandardPaths>
#include <QTextStream>

namespace
{

using IniGroups = QHash<QString, QHash<QString, QString>>;

// A tiny INI reader. QSettings mishandles section names that contain ':' (as
// "[Colors:Window]" does in KDE schemes), so we parse the handful of lines we
// need directly.
IniGroups parseIni(const QString &path)
{
    IniGroups groups;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return groups;

    QTextStream stream(&file);
    QString section;
    while (!stream.atEnd()) {
        const QString line = stream.readLine().trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')) || line.startsWith(QLatin1Char(';')))
            continue;
        if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
            section = line.mid(1, line.size() - 2).trimmed();
            continue;
        }
        const int equals = int(line.indexOf(QLatin1Char('=')));
        if (equals < 0)
            continue;
        const QString key = line.left(equals).trimmed();
        if (!key.isEmpty())
            groups[section][key] = line.mid(equals + 1).trimmed();
    }
    return groups;
}

// KDE colour schemes store colours as "r,g,b" (optionally "r,g,b,a").
QColor parseKdeColor(const QString &value)
{
    const QStringList parts = value.split(QLatin1Char(','));
    if (parts.size() >= 3) {
        bool okR = false;
        bool okG = false;
        bool okB = false;
        const int r = parts.at(0).trimmed().toInt(&okR);
        const int g = parts.at(1).trimmed().toInt(&okG);
        const int b = parts.at(2).trimmed().toInt(&okB);
        if (okR && okG && okB)
            return QColor(r, g, b);
    }
    return QColor(value);
}

QColor readColor(const IniGroups &groups, const QString &section, const QString &key, const QColor &fallback = QColor())
{
    const QString value = groups.value(section).value(key);
    if (value.isEmpty())
        return fallback;
    const QColor color = parseKdeColor(value);
    return color.isValid() ? color : fallback;
}

QColor blend(const QColor &a, const QColor &b, qreal t)
{
    const qreal r = a.redF() * (1 - t) + b.redF() * t;
    const qreal g = a.greenF() * (1 - t) + b.greenF() * t;
    const qreal bl = a.blueF() * (1 - t) + b.blueF() * t;
    return QColor::fromRgbF(float(r), float(g), float(bl));
}

} // namespace

QVector<SystemThemeProvider::Scheme> SystemThemeProvider::availableSchemes()
{
    QVector<Scheme> schemes;
    const QStringList roots = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation);
    for (const QString &root : roots) {
        const QDir directory(root + QStringLiteral("/color-schemes"));
        if (!directory.exists())
            continue;
        const QFileInfoList entries = directory.entryInfoList({QStringLiteral("*.colors")}, QDir::Files, QDir::Name);
        for (const QFileInfo &entry : entries) {
            const IniGroups groups = parseIni(entry.absoluteFilePath());
            QString name = groups.value(QStringLiteral("General")).value(QStringLiteral("Name"));
            if (name.isEmpty())
                name = entry.completeBaseName();
            schemes.append({name, entry.absoluteFilePath()});
        }
    }
    return schemes;
}

std::optional<ThemeDefinition> SystemThemeProvider::loadScheme(const QString &path)
{
    const IniGroups groups = parseIni(path);

    const QColor windowBg = readColor(groups, QStringLiteral("Colors:Window"), QStringLiteral("BackgroundNormal"));
    const QColor windowFg = readColor(groups, QStringLiteral("Colors:Window"), QStringLiteral("ForegroundNormal"));
    if (!windowBg.isValid() || !windowFg.isValid())
        return std::nullopt;

    const QColor viewBg = readColor(groups, QStringLiteral("Colors:View"), QStringLiteral("BackgroundNormal"), windowBg);
    const QColor viewFg = readColor(groups, QStringLiteral("Colors:View"), QStringLiteral("ForegroundNormal"), windowFg);
    const QColor alternate = readColor(groups,
                                       QStringLiteral("Colors:View"),
                                       QStringLiteral("BackgroundAlternate"),
                                       readColor(groups, QStringLiteral("Colors:Window"), QStringLiteral("BackgroundAlternate")));
    const QColor inactive = readColor(groups, QStringLiteral("Colors:Window"), QStringLiteral("ForegroundInactive"), blend(windowFg, windowBg, 0.45));
    const QColor link = readColor(groups,
                                  QStringLiteral("Colors:Window"),
                                  QStringLiteral("ForegroundLink"),
                                  readColor(groups, QStringLiteral("Colors:View"), QStringLiteral("ForegroundLink"), windowFg));
    const QColor visited = readColor(groups, QStringLiteral("Colors:Window"), QStringLiteral("ForegroundVisited"), link);

    ThemeDefinition theme;
    theme.name = groups.value(QStringLiteral("General")).value(QStringLiteral("Name"));
    if (theme.name.isEmpty())
        theme.name = QFileInfo(path).completeBaseName();
    theme.syntaxThemeName = QStringLiteral("System");

    theme.palette.setColor(QPalette::Window, windowBg);
    theme.palette.setColor(QPalette::WindowText, windowFg);
    theme.palette.setColor(QPalette::Base, viewBg);
    theme.palette.setColor(QPalette::AlternateBase, alternate.isValid() ? alternate : blend(viewBg, windowBg, 0.5));
    theme.palette.setColor(QPalette::Text, viewFg);
    theme.palette.setColor(QPalette::Button, readColor(groups, QStringLiteral("Colors:Button"), QStringLiteral("BackgroundNormal"), windowBg));
    theme.palette.setColor(QPalette::ButtonText, readColor(groups, QStringLiteral("Colors:Button"), QStringLiteral("ForegroundNormal"), windowFg));
    theme.palette.setColor(QPalette::Highlight, readColor(groups, QStringLiteral("Colors:Selection"), QStringLiteral("BackgroundNormal"), link));
    theme.palette.setColor(QPalette::HighlightedText, readColor(groups, QStringLiteral("Colors:Selection"), QStringLiteral("ForegroundNormal"), Qt::white));
    theme.palette.setColor(QPalette::ToolTipBase, readColor(groups, QStringLiteral("Colors:Tooltip"), QStringLiteral("BackgroundNormal"), viewBg));
    theme.palette.setColor(QPalette::ToolTipText, readColor(groups, QStringLiteral("Colors:Tooltip"), QStringLiteral("ForegroundNormal"), viewFg));
    theme.palette.setColor(QPalette::Link, link);
    theme.palette.setColor(QPalette::LinkVisited, visited);
    theme.palette.setColor(QPalette::PlaceholderText, inactive);

    const QColor border = blend(windowFg, windowBg, 0.7);
    theme.markdown.heading = windowFg;
    theme.markdown.weakText = inactive;
    theme.markdown.link = link;
    theme.markdown.codeBackground = theme.palette.color(QPalette::AlternateBase);
    theme.markdown.codeText = viewFg;
    theme.markdown.quoteBackground = viewBg;
    theme.markdown.quoteBar = border;
    theme.markdown.rule = border;
    theme.markdown.tableBorder = border;
    theme.markdown.tableHeaderBackground = theme.palette.color(QPalette::AlternateBase);
    theme.markdown.taskOpen = inactive;
    theme.markdown.taskDone = QColor(0x2e, 0xa0, 0x43);
    theme.markdown.currentLine = blend(viewBg, theme.palette.color(QPalette::Highlight), 0.14);

    const bool dark = windowBg.lightness() < 128;
    theme.syntax = dark ? ThemeDefinition::dark().syntax : ThemeDefinition::light().syntax;
    return theme;
}

QString SystemThemeProvider::currentSchemePath()
{
    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    const IniGroups groups = parseIni(configRoot + QStringLiteral("/kdeglobals"));
    const QString current = groups.value(QStringLiteral("General")).value(QStringLiteral("ColorScheme"));
    if (current.isEmpty())
        return QString();

    for (const Scheme &scheme : availableSchemes()) {
        if (scheme.name.compare(current, Qt::CaseInsensitive) == 0)
            return scheme.path;
    }
    return QString();
}

std::optional<ThemeDefinition> SystemThemeProvider::currentSystemTheme()
{
    const QString path = currentSchemePath();
    if (path.isEmpty())
        return std::nullopt;
    return loadScheme(path);
}