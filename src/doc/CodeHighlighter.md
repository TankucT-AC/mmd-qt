# CodeHighlighter

## 1. Class Overview

`CodeHighlighter` applies KF6 SyntaxHighlighting to the fenced code blocks of a
document produced by `MarkdownBuilder`. Because one document mixes languages —
one fence may be C++, the next Python — this is not a document-wide
`QSyntaxHighlighter`: each fenced region is highlighted on its own, with the
parser state reset at its first line. The language comes from the block's
`languageKey` property, set by the builder from the fence's info string.

## 2. Project Structure and Dependencies

- Owned by `viewer/MarkdownView`; called on every rebuild.
- Depends on the `KF6::SyntaxHighlighting` library (headers `AbstractHighlighter`, `Definition`, `Repository`, `Format`, `State`, `Theme`), on Qt6 Gui (`QTextCursor`, `QTextDocument`) and `core/Theme`.

## 3. Class Hierarchy and Role

Derives from `KSyntaxHighlighting::AbstractHighlighter`. The base runs a
language definition over lines and calls `applyFormat()` for each styled range;
this class applies those ranges into the `QTextDocument` by merging character
formats with a `QTextCursor`.

## 9. Public Methods

#### void setThemeDefinition(const ThemeDefinition &theme)

Stores the theme, resolves the syntax theme named by `syntaxThemeName` from the
repository (falling back to a default when unknown) and calls the base
`setTheme`. Takes effect on the next `highlight()`.

#### void highlight(QTextDocument *document)

Walks the document; for each run of code blocks it resolves the language
definition on the first line, resets the state, and highlights the block's text.
Blocks with an unknown language and everything outside code blocks are left
untouched.

## 10. Protected Virtual Methods / Event Handlers

#### void applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format) [override]

Base: `KSyntaxHighlighting::AbstractHighlighter`. Builds a `QTextCharFormat`
(foreground colour, bold, italic) from the syntax format against the active
theme and merges it into the current block at the given range. Empty formats are
ignored so plain text keeps the code block's own colour.

## 11. Ownership and Lifecycle

Plain object (no `Q_OBJECT`, no base ownership); `MarkdownView` owns it. The KF6
`Repository` is a value member.

## 12. Thread Safety

GUI-thread only: it edits the `QTextDocument` in place.

## 14. Inter-Class Interactions

- `MarkdownView::rebuild` calls `MarkdownBuilder::build` then `highlight()`.
- `MarkdownView::setRenderStyle` calls `setThemeDefinition` so code follows the app theme.

## 16. Usage Example

```cpp
CodeHighlighter highlighter;
highlighter.setThemeDefinition(ThemeDefinition::dark());
highlighter.highlight(&document); // only the fenced blocks change
```