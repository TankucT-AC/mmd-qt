# Theme

## 1. Class Overview

Everything visual in mmd-qt is described semantically: not "gray #6e7781" but
"weak text". `ThemeDefinition` is that description — one value that carries the
application palette, the markdown-specific roles the renderer needs, and the
name of the syntax-highlighting theme used for code.

Because a theme is a value, not code, switching themes never forks a code path.
Built-in themes are `light()` and `dark()`; arbitrary themes are data loaded from
JSON, which is the extension point for user themes.

## 2. Project Structure and Dependencies

- Used by `core/ThemeManager`, `core/MarkdownBuilder` (via `RenderStyle`), `editor/MarkdownEditor`, `editor/MarkdownHighlighter`, `viewer/CodeHighlighter`, `viewer/MarkdownView` and `app/MainWindow`.
- Depends on Qt6 Gui (`QPalette`, `QColor`) and Qt6 Core (`QJsonObject`, `QString`).
- No build-time dependency beyond Qt.

## 3. Class Hierarchy and Role

Two plain value types with no base and no `Q_OBJECT`:

- `MarkdownColors` — a plain aggregate of the markdown roles.
- `ThemeDefinition` — a `MarkdownColors` plus the palette, the name and the syntax theme name.

Both are cheap to copy, which is what makes `themeChanged(const ThemeDefinition &)`
a convenient signal payload.

## 6. Public Member Variables

`MarkdownColors`:

| Variable | Type | Description |
|----------|------|-------------|
| `heading` | `QColor` | Heading text (H1–H5). |
| `weakText` | `QColor` | H6 and meta information. |
| `link` | `QColor` | Link text. |
| `codeBackground` | `QColor` | Fenced blocks and inline code spans. |
| `codeText` | `QColor` | Code text where syntax highlighting does not override it. |
| `quoteBackground` | `QColor` | Block quote background. |
| `quoteBar` | `QColor` | The vertical bar beside a quote. |
| `rule` | `QColor` | Horizontal rules. |
| `tableBorder` | `QColor` | Table grid lines. |
| `tableHeaderBackground` | `QColor` | Table header row background. |
| `taskOpen` | `QColor` | Unchecked task marker. |
| `taskDone` | `QColor` | Checked task marker. |
| `currentLine` | `QColor` | The editor's active-line tint, distinct from both the plain background and `codeBackground`, so the caret's line stays findable inside a fence. |

`ThemeDefinition`:

| Variable | Type | Description |
|----------|------|-------------|
| `name` | `QString` | Identifier, e.g. `light` or a custom name. |
| `syntaxThemeName` | `QString` | KSyntaxHighlighting theme name, e.g. `Breeze Dark`. |
| `palette` | `QPalette` | The application palette applied to the whole window. |
| `markdown` | `MarkdownColors` | The markdown roles above. |

## 9. Public Methods

#### ThemeDefinition ThemeDefinition::light()

Returns the built-in light theme. Always valid and fully populated.

#### ThemeDefinition ThemeDefinition::dark()

Returns the built-in dark theme. Always valid and fully populated.

#### QJsonObject ThemeDefinition::toJson() const

Serialises the theme to a JSON object with `name`, `syntaxThemeName`, `palette`
(colours as `#rrggbb`) and `markdown` sub-objects. Used to write user themes.

#### std::optional\<ThemeDefinition\> ThemeDefinition::fromJson(const QJsonObject &object)

Parses a theme. Returns `std::nullopt` when required sub-objects or the name and
syntax theme are missing, so a malformed file is rejected rather than half-applied.

## 11. Ownership and Lifecycle

Pure value types. No ownership, no heap. The optional returned by `fromJson`
owns its value.

## 12. Thread Safety

Value types; safe to copy and read across threads. Mutating a shared instance is
the caller's responsibility (instances are passed by value).

## 13. QML Exposure

None — the project is Widgets-only.

## 14. Inter-Class Interactions

- `ThemeManager` produces and emits `ThemeDefinition` instances.
- `MainWindow::applyTheme` applies `palette` to `QApplication` and forwards the theme to the editor and viewer.
- `MarkdownView::setRenderStyle` stores one inside `RenderStyle` and hands it to the renderer and `CodeHighlighter`.

## 16. Usage Example

```cpp
ThemeDefinition theme = ThemeDefinition::light();
theme.markdown.link = QColor("#7c3aed");

const QJsonObject json = theme.toJson();
if (auto parsed = ThemeDefinition::fromJson(json))
    QApplication::setPalette(parsed->palette);
```