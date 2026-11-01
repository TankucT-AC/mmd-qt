// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"

#include "app/ScrollSync.h"
#include "core/ThemeManager.h"
#include "editor/MarkdownEditor.h"
#include "viewer/MarkdownView.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFile>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_themes(new ThemeManager(this))
{
    buildUi();
    buildMenus();
    connect(m_themes, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
    applyTheme(m_themes->currentTheme());
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

ThemeManager *MainWindow::themeManager() const
{
    return m_themes;
}

void MainWindow::buildUi()
{
    m_splitter = new QSplitter(Qt::Horizontal, this);

    m_editor = new MarkdownEditor(m_splitter);
    m_viewer = new MarkdownView(m_splitter);

    m_splitter->addWidget(m_editor);
    m_splitter->addWidget(m_viewer);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    setCentralWidget(m_splitter);

    connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
        m_viewer->setMarkdown(m_editor->toPlainText());
    });
    connect(m_splitter, &QSplitter::splitterMoved, this, [this] {
        m_viewer->renderNow();
    });
    m_scrollSync = new ScrollSync(m_editor, m_viewer, this);

    m_viewer->setMarkdown(m_editor->toPlainText());
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

    QMenu *themeMenu = viewMenu->addMenu(tr("&Theme"));
    auto *group = new QActionGroup(this);
    const QVector<QPair<QString, ThemeManager::Mode>> modes = {
        {tr("Follow &System"), ThemeManager::Mode::System},
        {tr("&Light"), ThemeManager::Mode::Light},
        {tr("&Dark"), ThemeManager::Mode::Dark},
        {tr("&Custom"), ThemeManager::Mode::Custom},
    };
    for (const auto &entry : modes) {
        QAction *action = group->addAction(entry.first);
        action->setCheckable(true);
        if (entry.second == m_themes->mode())
            action->setChecked(true);
        const ThemeManager::Mode mode = entry.second;
        connect(action, &QAction::toggled, this, [this, mode](bool checked) {
            if (checked)
                m_themes->setMode(mode);
        });
        themeMenu->addAction(action);
        if (entry.second == ThemeManager::Mode::Custom) {
            m_customThemeAction = action;
            action->setEnabled(false); // enabled once a custom theme file is loaded
        }
    }
    themeMenu->addSeparator();
    themeMenu->addAction(tr("Load Theme &File…"), this, &MainWindow::loadThemeFromFile);

    viewMenu->addSeparator();
    viewMenu->addAction(tr("&Quit"), QKeySequence(QStringLiteral("Ctrl+Q")), this, &QWidget::close);
}

void MainWindow::applyTheme(const ThemeDefinition &theme)
{
    QApplication::setPalette(theme.palette);
    m_editor->setTheme(theme);

    RenderStyle style = m_viewer->renderStyle();
    style.theme = theme;
    m_viewer->setRenderStyle(style);
}

MarkdownEditor *MainWindow::editor() const
{
    return m_editor;
}

MarkdownView *MainWindow::viewer() const
{
    return m_viewer;
}

void MainWindow::loadThemeFromFile()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Load Theme"), QString(), tr("Theme files (*.json)"));
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Load Theme"), tr("Cannot open %1.").arg(path));
        return;
    }

    QJsonParseError error = {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        QMessageBox::warning(this, tr("Load Theme"), tr("%1 is not a valid theme file: %2").arg(path, error.errorString()));
        return;
    }

    const std::optional<ThemeDefinition> theme = ThemeDefinition::fromJson(document.object());
    if (!theme) {
        QMessageBox::warning(this, tr("Load Theme"), tr("%1 is missing required theme fields.").arg(path));
        return;
    }

    m_themes->setCustomTheme(*theme);
    m_themes->setMode(ThemeManager::Mode::Custom);
    m_customThemeAction->setEnabled(true);
    m_customThemeAction->setChecked(true);
}
