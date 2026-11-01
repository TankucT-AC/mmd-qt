// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

class QFileSystemModel;
class QLabel;
class QModelIndex;
class QTreeView;

/*
 * The markdown files of a folder, so a whole set of notes can be browsed without
 * leaving the window. Everything that is not markdown is filtered out; the
 * folders stay, or there would be nothing to navigate.
 */
class FileTreePanel : public QWidget
{
    Q_OBJECT

public:
    explicit FileTreePanel(QWidget *parent = nullptr);

    void setRootDirectory(const QString &path);
    QString rootDirectory() const
    {
        return m_root;
    }

    // Reveals the file in the tree without opening it.
    void setCurrentFile(const QString &path);

signals:
    void fileActivated(const QString &path);

private:
    void handleIndex(const QModelIndex &index);
    void updateCaption();

    QFileSystemModel *m_model = nullptr;
    QTreeView *m_view = nullptr;
    QLabel *m_caption = nullptr;
    QString m_root;
};
