// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/GifObjectHandler.h"

#include "core/MarkdownBuilder.h"

#include <QMovie>
#include <QPainter>
#include <QSet>
#include <QTextBlock>
#include <QTextDocument>

GifObjectHandler::GifObjectHandler(QTextDocument *document, QObject *parent)
    : QObject(parent)
    , m_document(document)
{
}

void GifObjectHandler::setDocument(QTextDocument *document)
{
    if (m_document == document)
        return;

    // Positions refer to the document they were recorded in; they mean nothing
    // to the document that is about to show. drawObject() records them again.
    m_positions.clear();
    m_document = document;
}

void GifObjectHandler::setMaximumWidth(int width)
{
    m_maximumWidth = width;
}

QPixmap GifObjectHandler::scaledFrame(const QString &path, const QSize &size)
{
    if (m_scaledSize.value(path) == size && m_scaled.contains(path))
        return m_scaled.value(path);

    const QPixmap frame = movieFor(path)->currentPixmap();
    if (frame.isNull())
        return QPixmap();

    const QPixmap scaled = frame.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_scaled.insert(path, scaled);
    m_scaledSize.insert(path, size);
    return scaled;
}

void GifObjectHandler::syncToDocument()
{
    if (!m_document)
        return;

    QSet<QString> present;
    for (QTextBlock block = m_document->firstBlock(); block.isValid(); block = block.next()) {
        for (QTextBlock::Iterator it = block.begin(); !it.atEnd(); ++it) {
            const QTextCharFormat format = it.fragment().charFormat();
            if (format.objectType() == MarkdownBuilder::gifObjectType)
                present.insert(format.property(MarkdownBuilder::gifPathKey).toString());
        }
    }

    const QStringList known = m_movies.keys();
    for (const QString &path : known) {
        if (present.contains(path))
            continue;
        QMovie *movie = m_movies.take(path);
        movie->stop();
        movie->deleteLater();
        m_positions.remove(path);
        m_scaled.remove(path);
        m_scaledSize.remove(path);
    }
}

QMovie *GifObjectHandler::movieFor(const QString &path)
{
    const auto it = m_movies.constFind(path);
    if (it != m_movies.constEnd())
        return it.value();

    auto *movie = new QMovie(path, QByteArray(), this);
    m_movies.insert(path, movie);

    connect(movie, &QMovie::frameChanged, this, [this, path] {
        m_scaled.remove(path); // a new frame invalidates the scaled cache
        m_scaledSize.remove(path);
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
    const QSize natural = movieFor(path)->currentPixmap().size();
    if (natural.isEmpty())
        return QSizeF();
    if (m_maximumWidth > 0 && natural.width() > m_maximumWidth)
        return natural.scaled(m_maximumWidth, natural.height(), Qt::KeepAspectRatio);
    return natural;
}

void GifObjectHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    const QString path = format.property(MarkdownBuilder::gifPathKey).toString();
    if (path.isEmpty())
        return;

    m_positions.insert(path, posInDocument);
    const QPixmap frame = scaledFrame(path, rect.size().toSize());
    if (frame.isNull())
        return;
    painter->drawPixmap(rect.toRect(), frame);
}

bool GifObjectHandler::hasRunningAnimations() const
{
    for (QMovie *movie : m_movies) {
        if (movie->state() == QMovie::Running)
            return true;
    }
    return false;
}