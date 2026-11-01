// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

/*
 * Owns the file a window is showing: loads it, saves it, and reports when some
 * other program changes it on disk.
 *
 * Watching covers both in-place edits and the atomic-replace pattern (write to
 * a temporary file, then rename) used by most editors — the file's directory
 * is watched as well, so a replaced file is noticed and the watch re-armed.
 *
 * This is deliberately GUI-free: it never prompts. Deciding what to do about
 * an external change (reload silently, warn about unsaved edits) is the
 * window's job.
 */
class FileDocument : public QObject
{
    Q_OBJECT

public:
    explicit FileDocument(QObject *parent = nullptr);

    bool load(const QString &path, QString *error = nullptr);
    bool save(const QString &path, const QString &content, QString *error = nullptr);

    void close();

    QString filePath() const;
    QString content() const;
    bool exists() const;

signals:
    // The file changed on disk (edited or replaced) and was re-read into
    // content(). Not emitted for changes made through save().
    void externallyChanged();

private:
    void watchPath(const QString &path);
    void unwatchAll();
    void rearmWatch();
    void handleFileChanged(const QString &path);
    void handleDirectoryChanged(const QString &path);

    QFileSystemWatcher m_watcher;
    QString m_path;
    QString m_directory;
    QString m_content;
    bool m_saving = false;
};