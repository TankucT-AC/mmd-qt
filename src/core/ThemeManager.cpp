// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "core/ThemeManager.h"

#include "core/SystemThemeProvider.h"

#include <QGuiApplication>
#include <QStyleHints>

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, &ThemeManager::maybeEmitChange);
}

void ThemeManager::setMode(Mode mode)
{
    if (m_mode == mode)
        return;
    m_mode = mode;
    emit modeChanged(mode);
    emit themeChanged(currentTheme());
}

ThemeManager::Mode ThemeManager::mode() const
{
    return m_mode;
}

void ThemeManager::setCustomTheme(const ThemeDefinition &theme)
{
    m_custom = theme;
    if (m_mode == Mode::Custom)
        emit themeChanged(currentTheme());
}

ThemeDefinition ThemeManager::currentTheme() const
{
    return resolve();
}

void ThemeManager::maybeEmitChange()
{
    if (m_mode == Mode::System)
        emit themeChanged(currentTheme());
}

ThemeDefinition ThemeManager::resolve() const
{
    switch (m_mode) {
    case Mode::Light:
        return ThemeDefinition::light();
    case Mode::Dark:
        return ThemeDefinition::dark();
    case Mode::Custom:
        return m_custom;
    case Mode::System:
        break;
    }

    // Prefer the desktop's own colour scheme (e.g. a KDE Plasma scheme, like
    // Dolphin uses) when one is available; otherwise infer from the platform's
    // reported colour scheme.
    if (auto system = SystemThemeProvider::currentSystemTheme())
        return *system;

    const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
    return scheme == Qt::ColorScheme::Dark ? ThemeDefinition::dark() : ThemeDefinition::light();
}
