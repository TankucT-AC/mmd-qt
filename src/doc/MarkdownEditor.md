# MarkdownEditor

## 1. Class Overview

`MarkdownEditor` is the source pane of the two-pane view: a plain-text editor
with line numbers, current-line highlighting and markdown syntax highlighting. It
is the input side of the live preview and knows nothing about rendering markdown
— it only exposes its text, which the window forwards to the preview.

## 2. Project Structure and Dependencies

- Instantiated by `app/MainWindow`; its text changes drive `viewer/MarkdownView`.
- Depends on Qt6 Widgets (`QPlainTextEdit`), `core/FontManager` (code font) and `core/Theme` (colors), and owns a `editor/MarkdownHighlighter`.
- The line-number gutter is a private nested widget; no extra files.

## 3. Class Hierarchy and Role

Derives from `QPlainTextEdit`. `QWidget` provides the paint surface and event
routing; `QPlainTextEdit` provides the document, cursor and editing behaviour.
The gutter is implemented as a small child `QWidget` painted in
`lineNumberAreaPaintEvent`.

## 9. Public Methods

#### explicit MarkdownEditor(QWidget *parent = nullptr)

Creates the widget, installs the highlighter and the line-number area, and turns
off wrapping.

#### void setTheme(const ThemeDefinition &theme)

Applies the theme: re-highlights the document, sets the widget palette and
recomputes the current-line highlight.

#### void setMarkdown(const QString &markdown)

Replaces the whole document and re-highlights. Use this instead of
`setPlainText` for programmatic replacement: a `QSyntaxHighlighter` pass that
runs during `setPlainText` writes to a layout that is discarded afterwards, so
the text would otherwise appear unhighlighted until the next edit.

#### int lineNumberAreaWidth() const

The pixel width reserved for the gutter, sized to the number of digits in the
highest line number. Callers re-apply it as a viewport margin.

## 10. Protected Virtual Methods / Event Handlers

#### void resizeEvent(QResizeEvent *event) [override]

Base: `QPlainTextEdit`. Repositions the line-number gutter to match the new
contents rectangle. Calls the base implementation first.

## 11. Ownership and Lifecycle

Widget; parent-owned by the layout. The highlighter and the gutter are child
objects destroyed with it.

## 12. Thread Safety

GUI-thread only (all widgets are).

## 14. Inter-Class Interactions

- Owns a `MarkdownHighlighter` on its document.
- `MainWindow` connects `textChanged` to feed the preview and calls `setTheme` when the theme changes.

## 16. Usage Example

```cpp
auto *editor = new MarkdownEditor;
editor->setTheme(ThemeDefinition::dark());
editor->setMarkdown(QStringLiteral("# Hello\n"));
connect(editor, &QPlainTextEdit::textChanged, viewer, [&] { viewer->setMarkdown(editor->toPlainText()); });
```