// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QTextObjectInterface>

class QMovie;
class QTextDocument;

/*
 * Draws animated images (GIF) inside a QTextDocument via the
 * QTextObjectInterface mechanism — the document counterpart of a widget.
 *
 * Each distinct image has one QMovie driving its frames. When a frame
 * arrives, only the object's own region is invalidated, so playback never
 * forces a full-document relayout.
 */
class GifObjectHandler : public QObject, public QTextObjectInterface
{
    Q_OBJECT
    Q_INTERFACES(QTextObjectInterface)

public:
    explicit GifObjectHandler(QTextDocument *document, QObject *parent = nullptr);

    void setMaximumWidth(int width);

    QSizeF intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format) override;
    void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format) override;

    // True when at least one animated image is currently playing.
    bool hasRunningAnimations() const;

private:
    QMovie *movieFor(const QString &path);

    QTextDocument *m_document;
    int m_maximumWidth = 760;
    QHash<QString, QMovie *> m_movies;
    QHash<QString, int> m_positions;
};