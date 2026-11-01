# MarkdownHighlighter

## 1. Class Overview

`MarkdownHighlighter` colours the markdown *source* shown in the editor pane. It
is a lightweight visual aid for the person typing — headings, fences, spans,
links and list markers — not a full CommonMark grammar. Multi-line fenced code
blocks are styled with a block-state machine so an open fence keeps its shading
until the closing fence.

## 2. Project Structure and Dependencies

- Instantiated by `editor/MarkdownEditor` on its document.
- Depends on Qt6 Gui (`QSyntaxHighlighter`, `QTextCharFormat`, `QRegularExpression`) and `core/Theme`.
- All regular expressions are built once in the constructor, not per line.

## 3. Class Hierarchy and Role

Derives from `QSyntaxHighlighter`. The base class installs itself on a
`QTextDocument` and calls `highlightBlock()` when blocks change.

## 5. Enumerations

| Value | Description |
|-------|-------------|
| `StateNormal` (0) | The block is outside any fenced code block. |
| `StateInFence` (1) | The block is inside an open fenced code block; stored as the block state so the next block sees it. |

## 9. Public Methods

#### explicit MarkdownHighlighter(QTextDocument *document)

Attaches to `document` and compiles the highlighting rules.

#### void setTheme(const ThemeDefinition &theme)

Stores the theme and calls `rehighlight()`, so a theme switch recolours the whole
document.

## 10. Protected Virtual Methods / Event Handlers

#### void highlightBlock(const QString &text) [override]

Base: `QSyntaxHighlighter`. Applies inline and line-level rules, and drives the
fence state machine: a fence line toggles between `StateNormal` and
`StateInFence`, and content inside a fence is shaded as code.

## 11. Ownership and Lifecycle

`QObject` child of the document; created by `MarkdownEditor` with the document as
parent.

## 12. Thread Safety

GUI-thread only.

## 14. Inter-Class Interactions

- Created and themed by `MarkdownEditor`.

## 16. Usage Example

```cpp
QTextDocument document;
auto *highlighter = new MarkdownHighlighter(&document);
document.setPlainText(QStringLiteral("# Title\n"));
highlighter->setTheme(ThemeDefinition::light());
```