// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "core/ThemeManager.h"

#include <QHash>
#include <QMainWindow>

class QAction;
class QCloseEvent;
class QSplitter;
class FileDocument;
class MarkdownEditor;
class MarkdownView;
class ScrollSync;

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
    MarkdownEditor *editor() const;
    MarkdownView *viewer() const;
    FileDocument *fileDocument() const;

    // Opens a document by path; returns false and reports the error otherwise.
    bool openFile(const QString &path);

    // Saves the current document (prompting for a path when untitled).
    bool saveFile();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void buildMenus();
    void applyTheme(const ThemeDefinition &theme);
    void loadThemeFromFile();
    void restoreThemeSettings();
    void persistThemeSettings();

    void newFile();
    void openFileDialog();
    bool saveFileAs();
    bool maybeSave();
    void updateWindowTitle();
    void applyFileContent(const QString &content, const QString &path);
    void onExternalChange();

    QSplitter *m_splitter = nullptr;
    MarkdownEditor *m_editor = nullptr;
    MarkdownView *m_viewer = nullptr;
    ScrollSync *m_scrollSync = nullptr;
    FileDocument *m_file = nullptr;
    ThemeManager *m_themes = nullptr;
    QAction *m_toggleEditor = nullptr;
    QAction *m_toggleViewer = nullptr;
    QAction *m_customThemeAction = nullptr;
    QHash<int, QAction *> m_themeModeActions;
};