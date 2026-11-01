// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "viewer/CheckboxObjectHandler.h"

#include "core/MarkdownBuilder.h"

#include <QPainter>
#include <QPainterPath>
#include <QTextDocument>

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
    const qreal side = qMin(rect.width(), rect.height());
    const QRectF box(rect.x(), rect.center().y() - side / 2.0, side, side);
    const qreal radius = side * 0.22;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (checked) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(m_checkedFill);
        painter->drawRoundedRect(box, radius, radius);

        QPainterPath tick;
        tick.moveTo(box.left() + side * 0.22, box.top() + side * 0.53);
        tick.lineTo(box.left() + side * 0.42, box.top() + side * 0.72);
        tick.lineTo(box.left() + side * 0.79, box.top() + side * 0.28);
        QPen pen(m_check, qMax(1.2, side * 0.14));
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(tick);
    } else {
        painter->setPen(QPen(m_border, qMax(1.2, side * 0.11)));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(box.adjusted(0.6, 0.6, -0.6, -0.6), radius, radius);
    }

    painter->restore();
}