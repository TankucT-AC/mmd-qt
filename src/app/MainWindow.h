// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/ThemeManager.h"

#include <QMainWindow>

class QAction;
class QSplitter;
class QTextBrowser;
class MarkdownEditor;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    QWidget *editorPanel() const;
    QWidget *viewerPanel() const;
    QAction *toggleEditorAction() const;
    QAction *toggleViewerAction() const;
    ThemeManager *themeManager() const;

private:
    void buildUi();
    void buildMenus();
    void applyTheme(const ThemeDefinition &theme);
    void loadThemeFromFile();

    QSplitter *m_splitter = nullptr;
    MarkdownEditor *m_editor = nullptr;
    QTextBrowser *m_viewer = nullptr;
    QAction *m_toggleEditor = nullptr;
    QAction *m_toggleViewer = nullptr;
    QAction *m_customThemeAction = nullptr;
    ThemeManager *m_themes = nullptr;
};
