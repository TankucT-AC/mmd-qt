// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/MathObjectHandler.h"

#include "core/MarkdownBuilder.h"
#include "core/MathRenderer.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>
#include <QPen>
#include <QTextFormat>

MathObjectHandler::MathObjectHandler(QObject *parent)
    : QObject(parent)
{
}

void MathObjectHandler::setBodyFont(const QFont &font)
{
    m_bodyFont = font;
}

void MathObjectHandler::setEmSize(qreal pixels)
{
    if (pixels > 0)
        m_emSize = pixels;
}

void MathObjectHandler::setColor(const QColor &color)
{
    if (m_color == color)
        return;

    // The colour is baked into the cached bitmaps, so a theme switch starts
    // from a clean slate.
    m_color = color;
    MathRenderer::instance().clearCache();
}

void MathObjectHandler::setDevicePixelRatio(qreal ratio)
{
    m_devicePixelRatio = ratio > 0 ? ratio : 1.0;
}

QString MathObjectHandler::sourceOf(const QTextFormat &format) const
{
    return format.stringProperty(MarkdownBuilder::mathSourceKey);
}

bool MathObjectHandler::isDisplay(const QTextFormat &format) const
{
    return format.property(MarkdownBuilder::mathDisplayKey).toBool();
}

QImage MathObjectHandler::bitmapFor(const QTextFormat &format) const
{
    return MathRenderer::instance().image(sourceOf(format), isDisplay(format), m_emSize, m_color, m_devicePixelRatio);
}

QSizeF MathObjectHandler::sourceTextSize(const QString &source) const
{
    const QFontMetricsF metrics(m_bodyFont);
    return QSizeF(metrics.horizontalAdvance(source) + 8, metrics.height() + 4);
}

QSizeF MathObjectHandler::intrinsicSize(QTextDocument *, int, const QTextFormat &format)
{
    const QImage image = bitmapFor(format);
    if (!image.isNull())
        return QSizeF(image.size()) / image.devicePixelRatio();

    return sourceTextSize(sourceOf(format));
}

void MathObjectHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *, int, const QTextFormat &format)
{
    const QImage image = bitmapFor(format);
    if (image.isNull()) {
        // The renderer refused the formula -- a TeX error, or a glyph the font
        // backend could not produce. Show the source text instead of a hole:
        // degraded, but readable and honest about what is being shown.
        QFont italic = m_bodyFont;
        italic.setItalic(true);
        painter->save();
        painter->setFont(italic);
        painter->setPen(QPen(m_color));
        painter->drawText(rect, Qt::AlignCenter, sourceOf(format));
        painter->restore();
        return;
    }

    // Centre within the allotted rectangle: the layout rounds to whole pixels,
    // and the formula should not sit a pixel off its baseline box.
    const QSizeF logical = QSizeF(image.size()) / image.devicePixelRatio();
    const QPointF topLeft = rect.topLeft() + QPointF((rect.width() - logical.width()) / 2.0, (rect.height() - logical.height()) / 2.0);
    painter->drawImage(topLeft, image);
}
