// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMainWindow>

class QAction;
class QPlainTextEdit;
class QSplitter;
class QTextBrowser;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    QWidget *editorPanel() const;
    QWidget *viewerPanel() const;
    QAction *toggleEditorAction() const;
    QAction *toggleViewerAction() const;

private:
    void buildUi();
    void buildMenus();

    QSplitter *m_splitter = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QTextBrowser *m_viewer = nullptr;
    QAction *m_toggleEditor = nullptr;
    QAction *m_toggleViewer = nullptr;
};
