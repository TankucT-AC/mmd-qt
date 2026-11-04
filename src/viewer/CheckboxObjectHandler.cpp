// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/CheckboxObjectHandler.h"

#include "core/MarkdownBuilder.h"

#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>

#include <algorithm>

CheckboxObjectHandler::CheckboxObjectHandler(QObject *parent)
    : QObject(parent)
    , m_border(QColor(0x6e, 0x77, 0x81))
    , m_checkedFill(QColor(0x09, 0x69, 0xda))
    , m_check(Qt::white)
{
}

void CheckboxObjectHandler::setColors(const QColor &border, const QColor &checkedFill, const QColor &check)
{
    m_border = border;
    m_checkedFill = checkedFill;
    m_check = check;
}

QSizeF CheckboxObjectHandler::intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);
    const qreal side = format.property(MarkdownBuilder::checkboxSizeKey).toDouble();
    const qreal box = side > 0 ? side : 13.0;
    return QSizeF(box, box);
}

void CheckboxObjectHandler::drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format)
{
    Q_UNUSED(document);
    Q_UNUSED(posInDocument);

    const bool checked = format.property(MarkdownBuilder::checkboxCheckedKey).toBool();
    const qreal side = (std::min)(rect.width(), rect.height());
    // Zed-style: a square box, aligned to the text baseline area.
    const QRectF box(qRound(rect.x()), qRound(rect.center().y() - side / 2.0), qRound(side), qRound(side));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false); // square edges stay crisp

    if (checked) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_checkedFill);
        painter->drawRect(box);

        QPainterPath tick;
        tick.moveTo(box.left() + side * 0.24, box.top() + side * 0.52);
        tick.lineTo(box.left() + side * 0.43, box.top() + side * 0.71);
        tick.lineTo(box.left() + side * 0.77, box.top() + side * 0.30);
        QPen pen(m_check, (std::max)(1.4, side * 0.16));
        pen.setCapStyle(Qt::FlatCap);
        pen.setJoinStyle(Qt::MiterJoin);
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(tick);
    } else {
        painter->setPen(QPen(m_border, (std::max)(1.2, side * 0.09)));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(box.adjusted(0.5, 0.5, -0.5, -0.5));
    }

    painter->restore();
}