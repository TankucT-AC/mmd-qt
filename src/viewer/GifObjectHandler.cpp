// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/GifObjectHandler.h"

#include "core/MarkdownBuilder.h"

#include <QMovie>
#include <QPainter>
#include <QTextDocument>

GifObjectHandler::GifObjectHandler(QTextDocument *document, QObject *parent)
    : QObject(parent)
    , m_document(document)
{
}

void GifObjectHandler::setMaximumWidth(int width)
{
    m_maximumWidth = width;
}

QMovie *GifObjectHandler::movieFor(const QString &path)
{
    const auto it = m_movies.constFind(path);
    if (it != m_movies.constEnd())
        return it.value();

    auto *movie = new QMovie(path, QByteArray(), this);
    const QSize natural = movie->currentPixmap().size();
    if (m_maximumWidth > 0 && natural.width() > m_maximumWidth) {
        movie->setScaledSize(natural.scaled(m_maximumWidth, natural.height(), Qt::KeepAspectRatio));
    }
    m_movies.insert(path, movie);

    connect(movie, &QMovie::frameChanged, this, [this, path] {
        const int position = m_positions.value(path, -1);
        if (position >= 0)
            m_document->markContentsDirty(position, 1);
    });
    movie->start();
    return movie;
}

QSizeF GifObjectHandler::intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);
    const QString path = format.property(MarkdownBuilder::gifPathKey).toString();
    if (path.isEmpty())
        return QSizeF();

    // Starting the movie here is what makes the first frame available for
    // measurement; drawObject then reuses the cached instance.
    return movieFor(path)->currentPixmap().size();
}

void GifObjectHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    const QString path = format.property(MarkdownBuilder::gifPathKey).toString();
    if (path.isEmpty())
        return;

    QMovie *movie = movieFor(path);
    m_positions.insert(path, posInDocument);
    painter->drawPixmap(rect.toRect(), movie->currentPixmap());
}

bool GifObjectHandler::hasRunningAnimations() const
{
    for (QMovie *movie : m_movies) {
        if (movie->state() == QMovie::Running)
            return true;
    }
    return false;
}