// SPDX-FileCopyrightText: 2026 Total Markdown contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "app/SidebarTitleBar.h"

#include <QAbstractButton>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>

namespace
{

constexpr int kButtonSide = 24;
// The square the glyph is laid out in, centred in the button.
constexpr int kGlyphSide = 16;
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
            painter.drawLine(QPointF(3.5, 3.5), QPointF(12.5, 12.5));
            painter.drawLine(QPointF(12.5, 3.5), QPointF(3.5, 12.5));
            return;
        }

        // A square with a hole, the way Windows draws "restore down": the front
        // square covers the back one, and the back square's border is cut short
        // where it passes underneath. The cut is made with a clip path instead
        // of an opaque fill, so it stays correct over the hover tint and in any
        // theme without knowing the background colour.
        const QRectF back(1.0, 1.0, 9.0, 9.0);
        const QRectF front(6.0, 6.0, 9.0, 9.0);

        const qreal margin = kGlyphPen / 2.0 + 0.2; // keep the cut clear of the front stroke
        QPainterPath visible;
        visible.addRect(QRectF(0.0, 0.0, kGlyphSide, kGlyphSide));
        visible.addRect(front.adjusted(-margin, -margin, margin, margin)); // even-odd hole
        painter.setClipPath(visible);
        painter.drawRoundedRect(back, 2.0, 2.0);
        painter.setClipping(false);
        painter.drawRoundedRect(front, 2.0, 2.0);
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
