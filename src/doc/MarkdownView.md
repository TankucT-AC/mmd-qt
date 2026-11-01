# MarkdownView

## 1. Class Overview

`MarkdownView` is the preview pane. It renders markdown source into its
`QTextDocument` and keeps the result up to date as the source changes: repeated
`setMarkdown()` calls inside a debounce window collapse into a single rebuild, so
a burst of keystrokes costs one render rather than one per key. The scroll
position is preserved across a rebuild by anchoring on the block currently at the
top of the viewport.

The document is rebuilt in place rather than swapped, which keeps the animated
image handler registration valid.

## 2. Project Structure and Dependencies

- Instantiated by `app/MainWindow`.
- Depends on Qt6 Widgets (`QTextBrowser`), `core/MarkdownBuilder`, `core/FontManager`, and owns `viewer/CodeHighlighter` and `viewer/GifObjectHandler`.

## 3. Class Hierarchy and Role

Derives from `QTextBrowser` (a read-only `QTextEdit` with hyperlink navigation).
Hyperlinks open externally. The GIF `QTextObjectInterface` handler is registered
on the document layout at construction.

## 7. Signals

#### void rendered()

Emitted after each rebuild completes. Tests use it to observe coalescing.

## 9. Public Methods

#### explicit MarkdownView(QWidget *parent = nullptr)

Creates the view, registers the GIF handler and seeds the style with the bundled fonts.

#### void setRenderStyle(const RenderStyle &style)

Stores the style, updates the GIF width cap and the code highlighter's theme,
applies the palette and re-renders immediately.

#### const RenderStyle &renderStyle() const

The current style; callers mutate a copy and pass it back (used by `MainWindow::applyTheme`).

#### void setBaseDir(const QString &directory)

Sets the directory against which relative image and link references resolve, then re-renders.

#### void setMarkdown(const QString &markdown)

Records the source and schedules a render; calls inside the debounce interval are absorbed. With an interval of zero the render is synchronous.

#### void renderNow()

Renders immediately, bypassing the debounce (used after a splitter resize).

#### int renderInterval() const / void setRenderInterval(int milliseconds)

The debounce interval in milliseconds. `0` renders synchronously, which is what the tests use.

## 11. Ownership and Lifecycle

Widget; parent-owned. The code highlighter is an owned child; the GIF handler is a child of the view and is registered, not owned, by the document layout.

## 12. Thread Safety

GUI-thread only. Re-rendering touches the document and the scrollbar.

## 14. Inter-Class Interactions

- `MainWindow` connects the editor's `textChanged` to `setMarkdown`, calls `renderNow()` on splitter movement, and calls `setRenderStyle` on theme changes.
- Calls `MarkdownBuilder::build` and `CodeHighlighter::highlight` on each rebuild.

## 16. Usage Example

```cpp
auto *view = new MarkdownView;
view->setRenderInterval(0);
view->setMarkdown(QStringLiteral("# Title\n"));
```