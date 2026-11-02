// SPDX-FileCopyrightText: 2026 mmd-qt contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/SidebarTitleBar.h"

#include <QAbstractButton>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>

namespace
{

constexpr int kButtonSide = 26;
// The square the glyph is laid out in, centred in the button.
constexpr int kGlyphSide = 18;
constexpr qreal kGlyphPen = 1.4;

/*
 * A flat button that paints its own glyph.
 *
 * A QIcon built from an 18 px pixmap is a bitmap at that size: on a 2x screen it
 * is stretched to 36 device pixels and goes soft. Painting on the widget keeps
 * the glyph in device coordinates at whatever ratio the screen has, which is how
 * the code box's copy button stays sharp, so this follows the same recipe.
 */
class GlyphButton : public QAbstractButton
{
public:
    enum class Glyph {
        Float,
        Close,
    };

    GlyphButton(Glyph glyph, QWidget *parent)
        : QAbstractButton(parent)
        , m_glyph(glyph)
    {
        setAttribute(Qt::WA_Hover);
        setFixedSize(kButtonSide, kButtonSide);
    }

protected:
    void enterEvent(QEnterEvent *event) override
    {
        QAbstractButton::enterEvent(event);
        update();
    }

    void leaveEvent(QEvent *event) override
    {
        QAbstractButton::leaveEvent(event);
        update();
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QColor ink = palette().color(QPalette::WindowText);

        if (isDown() || underMouse()) {
            QColor tint = ink;
            tint.setAlpha(isDown() ? 48 : 24);
            painter.fillRect(rect(), tint);
        }

        painter.setPen(QPen(ink, kGlyphPen));
        painter.setBrush(Qt::NoBrush);
        painter.translate((width() - kGlyphSide) / 2.0, (height() - kGlyphSide) / 2.0);

        if (m_glyph == Glyph::Close) {
            painter.drawLine(QPointF(4.0, 4.0), QPointF(14.0, 14.0));
            painter.drawLine(QPointF(14.0, 4.0), QPointF(4.0, 14.0));
            return;
        }

        // Two overlapping sheets: the docked pane, and the one that leaves.
        constexpr qreal kSheet = 8.5;
        painter.drawRoundedRect(QRectF(1.0, 1.0, kSheet, kSheet), 2.0, 2.0);
        painter.drawRoundedRect(QRectF(8.5, 8.5, kSheet, kSheet), 2.0, 2.0);
    }

private:
    Glyph m_glyph;
};

} // namespace

SidebarTitleBar::SidebarTitleBar(QDockWidget *dock, const QString &title, QWidget *parent)
    : QWidget(parent)
{
    auto *label = new QLabel(title, this);
    label->setContentsMargins(6, 0, 0, 0);

    auto *floatButton = new GlyphButton(GlyphButton::Glyph::Float, this);
    floatButton->setObjectName(QStringLiteral("sidebarFloatButton"));
    floatButton->setToolTip(tr("Float the sidebar"));
    m_float = floatButton;

    auto *closeButton = new GlyphButton(GlyphButton::Glyph::Close, this);
    closeButton->setObjectName(QStringLiteral("sidebarCloseButton"));
    closeButton->setToolTip(tr("Close the sidebar"));
    m_close = closeButton;

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 2, 4, 2);
    layout->setSpacing(2);
    layout->addWidget(label);
    layout->addStretch(1);
    layout->addWidget(m_float);
    layout->addWidget(m_close);

    connect(m_close, &QAbstractButton::clicked, dock, &QWidget::hide);
    connect(m_float, &QAbstractButton::clicked, dock, [dock] {
        dock->setFloating(!dock->isFloating());
    });
}
