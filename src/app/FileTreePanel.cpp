// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/FileTreePanel.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QLabel>
#include <QTreeView>
#include <QVBoxLayout>

namespace
{

// The extensions a markdown browser should offer. Anything else stays hidden.
const QStringList &markdownFilters()
{
    static const QStringList filters = {
        QStringLiteral("*.md"),
        QStringLiteral("*.markdown"),
        QStringLiteral("*.mdown"),
        QStringLiteral("*.mkd"),
    };
    return filters;
}

} // namespace

FileTreePanel::FileTreePanel(QWidget *parent)
    : QWidget(parent)
    , m_model(new QFileSystemModel(this))
    , m_view(new QTreeView(this))
    , m_caption(new QLabel(this))
{
    m_model->setFilter(QDir::AllDirs | QDir::Files | QDir::NoDotAndDotDot);
    m_model->setNameFilters(markdownFilters());
    m_model->setNameFilterDisables(false); // hide what does not match
    m_model->setReadOnly(true);

    m_view->setModel(m_model);
    m_view->setHeaderHidden(true);
    m_view->setUniformRowHeights(true);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    for (int column = 1; column < m_model->columnCount(); ++column)
        m_view->hideColumn(column);

    m_caption->setContentsMargins(6, 4, 6, 4);
    m_caption->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_caption);
    layout->addWidget(m_view, 1);

    // A click opens the file (this is a browser, not a form); Enter does the
    // same for keyboard users.
    connect(m_view, &QTreeView::clicked, this, &FileTreePanel::handleIndex);
    connect(m_view, &QTreeView::activated, this, &FileTreePanel::handleIndex);

    updateCaption();
}

void FileTreePanel::updateCaption()
{
    if (m_root.isEmpty()) {
        m_caption->setText(tr("No folder opened yet.\nUse File ▸ Open Folder…"));
        return;
    }
    m_caption->setText(QDir(m_root).dirName());
}

void FileTreePanel::setRootDirectory(const QString &path)
{
    if (path.isEmpty() || path == m_root)
        return;

    m_root = path;
    m_view->setRootIndex(m_model->setRootPath(path));
    updateCaption();
}

void FileTreePanel::setCurrentFile(const QString &path)
{
    if (path.isEmpty() || m_root.isEmpty())
        return;

    const QModelIndex index = m_model->index(path);
    if (!index.isValid())
        return;

    m_view->setCurrentIndex(index); // currentChanged, not clicked: no reload
    m_view->scrollTo(index, QAbstractItemView::PositionAtCenter);
}

void FileTreePanel::handleIndex(const QModelIndex &index)
{
    if (!index.isValid() || m_model->isDir(index))
        return;

    const QString path = m_model->filePath(index);
    if (!path.isEmpty())
        emit fileActivated(path);
}
