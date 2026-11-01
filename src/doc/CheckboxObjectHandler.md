# CheckboxObjectHandler

## 1. Class Overview

`CheckboxObjectHandler` draws task-list checkboxes in the preview. The earlier
approach used the ☐ / ☑ glyphs, whose shape and colour did not match the theme;
here the box is drawn to the palette — a rounded outline when unchecked, and an
accent-filled box with a tick when checked.

It is a `QTextObjectInterface`, the document-side counterpart of a widget, so
the checkbox is a real object inside the text flow rather than a font glyph.

## 2. Project Structure and Dependencies

- Registered by `viewer/MarkdownView` on its document layout for `MarkdownBuilder::checkboxObjectType`.
- Depends on Qt6 Gui (`QPainter`, `QPainterPath`, `QTextObjectInterface`) and `core/MarkdownBuilder` for the object type and its property keys.

## 3. Class Hierarchy and Role

Derives from `QObject` (for parenting) and implements `QTextObjectInterface`
(declared with `Q_INTERFACES`). `MarkdownBuilder` inserts the object; the
document layout calls back here to size and paint it.

## 9. Public Methods

#### explicit CheckboxObjectHandler(QObject *parent = nullptr)

Creates the handler with sensible default colours (muted border, accent fill,
white tick).

#### void setColors(const QColor &border, const QColor &checkedFill, const QColor &check)

Sets the drawing colours. `MarkdownView` calls it on every theme change with the
theme's weak-text colour, link colour and base colour.

#### QSizeF intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format) [override]

Returns a square box whose side comes from the object's `checkboxSizeKey`
property, set by the builder from the body font height.

#### void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format) [override]

Draws the box. When the object's `checkboxCheckedKey` property is set, it fills
a rounded square with the accent colour and strokes a tick; otherwise it strokes
only an outline.

## 11. Ownership and Lifecycle

`QObject` child of the view; destroyed with it. The document is not owned.

## 12. Thread Safety

GUI-thread only: it paints into the document layout.

## 14. Inter-Class Interactions

- `MarkdownBuilder` inserts the checkbox objects for `[ ]` / `[x]` task items.
- `MarkdownView` registers the handler and colours it from the active theme.

## 16. Usage Example

```cpp
auto *handler = new CheckboxObjectHandler(view);
handler->setColors(theme.markdown.weakText, theme.markdown.link, theme.palette.color(QPalette::Base));
document->documentLayout()->registerHandler(MarkdownBuilder::checkboxObjectType, handler);
```