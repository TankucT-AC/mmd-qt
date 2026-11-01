// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/ChipObjectHandler.h"

#include "core/MarkdownBuilder.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QTextDocument>

#include <algorithm>

namespace
{
constexpr qreal kPadH = 7.0;
constexpr qreal kPadV = 2.0;
}

ChipObjectHandler::ChipObjectHandler(QObject *parent)
    : QObject(parent)
    , m_background(QColor(0xf3, 0xf4, 0xf6))
    , m_border(QColor(0xd0, 0xd7, 0xde))
    , m_text(QColor(0x1f, 0x23, 0x28))
{
}

void ChipObjectHandler::setColors(const QColor &background, const QColor &border, const QColor &text)
{
    m_background = background;
    m_border = border;
    m_text = text;
}

QSizeF ChipObjectHandler::intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);
    QFont font = format.toCharFormat().font();
    if (font.pointSizeF() <= 0)
        font.setPointSizeF(11.0);
    const QFontMetricsF metrics(font);
    const QString text = format.property(MarkdownBuilder::chipTextKey).toString();
    const qreal width = metrics.horizontalAdvance(text) + 2 * kPadH;
    const qreal height = metrics.height() + 2 * kPadV;
    return QSizeF((std::max)(width, height), height);
}

void ChipObjectHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);

    const QString text = format.property(MarkdownBuilder::chipTextKey).toString();
    if (text.isEmpty())
        return;

    QFont font = format.toCharFormat().font();
    if (font.pointSizeF() <= 0)
        font.setPointSizeF(11.0);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QRectF box = rect.adjusted(0.5, 0.5, -0.5, -0.5);
    painter->setPen(QPen(m_border, 1.0));
    painter->setBrush(m_background);
    painter->drawRoundedRect(box, 4.0, 4.0);

    painter->setPen(m_text);
    painter->setFont(font);
    painter->drawText(rect, Qt::AlignCenter, text);

    painter->restore();
}