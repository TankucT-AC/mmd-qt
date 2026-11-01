# GifObjectHandler

## 1. Class Overview

`GifObjectHandler` draws animated images inside a `QTextDocument` through the
`QTextObjectInterface` mechanism — the document's counterpart of a widget. Each
distinct image has one `QMovie` driving its frames; when a frame arrives only the
object's own region is invalidated, so playback never forces a full-document
relayout. Static images are drawn by the text layout itself and do not use this
handler; only formats tagged with `MarkdownBuilder::gifObjectType` do.

## 2. Project Structure and Dependencies

- Registered by `viewer/MarkdownView` on its document layout for `MarkdownBuilder::gifObjectType`.
- Depends on Qt6 Gui (`QMovie`, `QPainter`, `QTextObjectInterface`) and `core/MarkdownBuilder` for the object type and property key.

## 3. Class Hierarchy and Role

Derives from `QObject` and implements `QTextObjectInterface` (declared with
`Q_INTERFACES`). `QObject` gives it parenting and signal handling; the interface
lets the document layout measure and paint the object.

## 9. Public Methods

#### explicit GifObjectHandler(QTextDocument *document, QObject *parent = nullptr)

Stores the document the objects live in.

#### void setMaximumWidth(int width)

Caps the displayed width; wider images are scaled down keeping the aspect ratio.

#### QSizeF intrinsicSize(QTextDocument *document, int posInDocument, const QTextFormat &format) [override]

Returns the scaled size of the image's current frame, so the layout reserves the
right box. Starts the movie on first call, which is what makes the first frame
available for measurement.

#### void drawObject(QPainter *painter, const QRectF &rect, QTextDocument *document, int posInDocument, const QTextFormat &format) [override]

Draws the movie's current frame scaled into `rect` and records the object's
position, so a later frame change can invalidate exactly that region.

#### bool hasRunningAnimations() const

True when at least one tracked movie is running. Used by tests.

## 11. Ownership and Lifecycle

`QObject` child of the view. The `QMovie` instances are children of the handler
and are destroyed with it. The document is not owned; it outlives the handler
(`MarkdownView` owns both and creates the handler with the document already
present).

## 12. Thread Safety

GUI-thread only: it paints and mutates the document's dirty regions.

## 14. Inter-Class Interactions

- `MarkdownBuilder` emits `gifObjectType` formats for animated images.
- `MarkdownView` registers the handler and calls `setMaximumWidth` from the render style.

## 16. Usage Example

```cpp
auto *handler = new GifObjectHandler(document, view);
document->documentLayout()->registerHandler(MarkdownBuilder::gifObjectType, handler);
handler->setMaximumWidth(760);
```