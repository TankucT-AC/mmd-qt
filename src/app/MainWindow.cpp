// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/MainWindow.h"

#include "app/ScrollSync.h"
#include "core/FileDocument.h"
#include "core/SystemThemeProvider.h"
#include "core/ThemeManager.h"
#include "editor/MarkdownEditor.h"
#include "viewer/MarkdownView.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>

#include <array>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_file(new FileDocument(this))
    , m_themes(new ThemeManager(this))
{
    buildUi();
    buildMenus();
    connect(m_themes, &ThemeManager::themeChanged, this, &MainWindow::applyTheme);
    connect(m_file, &FileDocument::externallyChanged, this, &MainWindow::onExternalChange);
    connect(m_editor->document(), &QTextDocument::modificationChanged, this, &MainWindow::updateWindowTitle);

    applyTheme(m_themes->currentTheme());
    updateWindowTitle();
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

MarkdownEditor *MainWindow::editor() const
{
    return m_editor;
}

MarkdownView *MainWindow::viewer() const
{
    return m_viewer;
}

FileDocument *MainWindow::fileDocument() const
{
    return m_file;
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
        m_viewer->scheduleRerender();
    });
    m_scrollSync = new ScrollSync(m_editor, m_viewer, this);

    m_viewer->setMarkdown(m_editor->toPlainText());
}

void MainWindow::buildMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction(tr("&Open…"), QKeySequence::Open, this, &MainWindow::openFileDialog);
    fileMenu->addAction(tr("&Save"), QKeySequence::Save, this, &MainWindow::saveFile);
    fileMenu->addAction(tr("Save &As…"), QKeySequence::SaveAs, this, &MainWindow::saveFileAs);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("&Quit"), QKeySequence(QStringLiteral("Ctrl+Q")), this, &QWidget::close);

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
    struct ThemeChoice {
        QString label;
        ThemeManager::Mode mode;
    };
    const std::array<ThemeChoice, 4> modes = {{
        {tr("Follow &System"), ThemeManager::Mode::System},
        {tr("&Light"), ThemeManager::Mode::Light},
        {tr("&Dark"), ThemeManager::Mode::Dark},
        {tr("&Custom"), ThemeManager::Mode::Custom},
    }};
    for (const ThemeChoice &entry : modes) {
        QAction *action = group->addAction(entry.label);
        action->setCheckable(true);
        if (entry.mode == m_themes->mode())
            action->setChecked(true);
        const ThemeManager::Mode mode = entry.mode;
        connect(action, &QAction::toggled, this, [this, mode](bool checked) {
            if (checked)
                m_themes->setMode(mode);
        });
        themeMenu->addAction(action);
        if (entry.mode == ThemeManager::Mode::Custom) {
            m_customThemeAction = action;
            action->setEnabled(false); // enabled once a custom theme file is loaded
        }
    }
    themeMenu->addSeparator();
    themeMenu->addAction(tr("Load Theme &File…"), this, &MainWindow::loadThemeFromFile);

    // Discovered desktop colour schemes (e.g. KDE Plasma *.colors), if any.
    const QVector<SystemThemeProvider::Scheme> schemes = SystemThemeProvider::availableSchemes();
    if (!schemes.isEmpty()) {
        QMenu *systemMenu = themeMenu->addMenu(tr("System &Schemes"));
        for (const SystemThemeProvider::Scheme &scheme : schemes) {
            const QString path = scheme.path;
            QAction *action = systemMenu->addAction(scheme.name);
            connect(action, &QAction::triggered, this, [this, path] {
                if (const std::optional<ThemeDefinition> theme = SystemThemeProvider::loadScheme(path)) {
                    m_themes->setCustomTheme(*theme);
                    m_themes->setMode(ThemeManager::Mode::Custom);
                    m_customThemeAction->setEnabled(true);
                    m_customThemeAction->setChecked(true);
                }
            });
        }
    }
}

void MainWindow::applyTheme(const ThemeDefinition &theme)
{
    QApplication::setPalette(theme.palette);
    m_editor->setTheme(theme);

    RenderStyle style = m_viewer->renderStyle();
    style.theme = theme;
    m_viewer->setRenderStyle(style);
}

void MainWindow::applyFileContent(const QString &content, const QString &path)
{
    m_viewer->setBaseDir(path.isEmpty() ? QString() : QFileInfo(path).absolutePath());
    m_editor->setMarkdown(content);
    m_editor->document()->setModified(false);
    m_viewer->setMarkdown(content);
    updateWindowTitle();
}

void MainWindow::updateWindowTitle()
{
    const QString name = m_file->filePath().isEmpty() ? tr("Untitled") : QFileInfo(m_file->filePath()).fileName();
    const QString modified = m_editor->document()->isModified() ? QStringLiteral("*") : QString();
    setWindowTitle(tr("%1%2 — mmd").arg(modified, name));
}

bool MainWindow::openFile(const QString &path)
{
    QString error;
    if (!m_file->load(path, &error)) {
        QMessageBox::warning(this, tr("Open File"), tr("Cannot open %1:\n%2").arg(path, error));
        return false;
    }
    applyFileContent(m_file->content(), m_file->filePath());
    return true;
}

void MainWindow::openFileDialog()
{
    if (!maybeSave())
        return;
    const QString path = QFileDialog::getOpenFileName(this, tr("Open Markdown"), QString(), tr("Markdown files (*.md *.markdown *.mkd);;All files (*)"));
    if (!path.isEmpty())
        openFile(path);
}

void MainWindow::newFile()
{
    if (!maybeSave())
        return;
    m_file->close();
    applyFileContent(QString(), QString());
}

bool MainWindow::saveFile()
{
    if (m_file->filePath().isEmpty())
        return saveFileAs();

    QString error;
    if (!m_file->save(m_file->filePath(), m_editor->toPlainText(), &error)) {
        QMessageBox::warning(this, tr("Save File"), tr("Cannot save %1:\n%2").arg(m_file->filePath(), error));
        return false;
    }
    m_editor->document()->setModified(false);
    updateWindowTitle();
    return true;
}

bool MainWindow::saveFileAs()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save Markdown"), m_file->filePath(), tr("Markdown files (*.md);;All files (*)"));
    if (path.isEmpty())
        return false;

    QString error;
    if (!m_file->save(path, m_editor->toPlainText(), &error)) {
        QMessageBox::warning(this, tr("Save File"), tr("Cannot save %1:\n%2").arg(path, error));
        return false;
    }
    m_editor->document()->setModified(false);
    m_viewer->setBaseDir(QFileInfo(path).absolutePath());
    updateWindowTitle();
    return true;
}

bool MainWindow::maybeSave()
{
    if (!m_editor->document()->isModified())
        return true;

    const QMessageBox::StandardButton answer = QMessageBox::warning(this,
                                                                    tr("mmd"),
                                                                    tr("The document has unsaved changes.\nDo you want to save them?"),
                                                                    QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save)
        return saveFile();
    return answer != QMessageBox::Cancel;
}

void MainWindow::onExternalChange()
{
    if (m_editor->document()->isModified()) {
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this,
            tr("File Changed"),
            tr("%1 changed on disk, but you have unsaved edits.\nReload and lose your edits?").arg(QFileInfo(m_file->filePath()).fileName()),
            QMessageBox::Yes | QMessageBox::No);
        if (answer != QMessageBox::Yes)
            return;
    }
    applyFileContent(m_file->content(), m_file->filePath());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
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