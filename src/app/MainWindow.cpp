// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QTextBrowser>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    buildMenus();
    setWindowTitle(tr("mmd"));
    resize(1200, 800);
}

QWidget *MainWindow::editorPanel() const
{
    return m_editor;
}

QWidget *MainWindow::viewerPanel() const
{
    return m_viewer;
}

QAction *MainWindow::toggleEditorAction() const
{
    return m_toggleEditor;
}

QAction *MainWindow::toggleViewerAction() const
{
    return m_toggleViewer;
}

void MainWindow::buildUi()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // Placeholders; replaced by MarkdownEditor and MarkdownView as they land.
    m_editor = new QPlainTextEdit(m_splitter);
    m_editor->setPlaceholderText(tr("Markdown source"));
    m_viewer = new QTextBrowser(m_splitter);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_viewer);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    setCentralWidget(m_splitter);
}

void MainWindow::buildMenus()
{
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    m_toggleEditor = viewMenu->addAction(tr("&Editor"));
    m_toggleEditor->setCheckable(true);
    m_toggleEditor->setChecked(true);
    m_toggleEditor->setShortcut(QStringLiteral("F9"));
    connect(m_toggleEditor, &QAction::toggled, m_editor, &QWidget::setVisible);

    m_toggleViewer = viewMenu->addAction(tr("&Preview"));
    m_toggleViewer->setCheckable(true);
    m_toggleViewer->setChecked(true);
    m_toggleViewer->setShortcut(QStringLiteral("F10"));
    connect(m_toggleViewer, &QAction::toggled, m_viewer, &QWidget::setVisible);

    viewMenu->addSeparator();
    viewMenu->addAction(tr("&Quit"), QKeySequence(QStringLiteral("Ctrl+Q")), this, &QWidget::close);
}
