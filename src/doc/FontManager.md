# FontManager

## 1. Class Overview

`FontManager` centralizes typography. It offers markdown prose, code and
heading fonts in two interchangeable sets: **Bundled** (Inter and JetBrains
Mono, both OFL, shipped as Qt resources so every machine looks identical) and
**System** (the platform's proportional and fixed fonts). Heading sizes follow a
modular scale, so the hierarchy stays proportional if the base size changes.

## 2. Project Structure and Dependencies

- Used by `core/MarkdownBuilder` (through `RenderStyle`), `editor/MarkdownEditor` and `viewer/MarkdownView`.
- Depends on Qt6 Gui (`QFont`, `QFontDatabase`).
- Font files are compiled in via `third_party/fonts/fonts.qrc` and registered with `Q_INIT_RESOURCE(fonts)` from within this translation unit — the linker would otherwise drop the generated resource initialiser in the static library.

## 3. Class Hierarchy and Role

A plain class with no base and no `Q_OBJECT`. Copyable and cheap.

## 5. Enumerations

| Value | Description |
|-------|-------------|
| `FontSet::Bundled` | Inter for prose, JetBrains Mono for code (default). |
| `FontSet::System` | `QFontDatabase::systemFont(GeneralFont/FixedFont)`. |

## 9. Public Methods

#### static QStringList ensureBundledFontsLoaded()

Registers the bundled fonts with the application font database. Idempotent and
cheap after the first call. Returns the families that are available afterwards.
Must be called before any bundled font is used; the constructor calls it.

#### explicit FontManager(FontSet set = FontSet::Bundled)

Constructs a manager for a font set and ensures the bundled faces are registered.

#### void setFontSet(FontSet set) / FontSet fontSet() const

Selects or reports the active set. Changing it later affects subsequently
returned fonts; callers re-fetch and re-apply.

#### QFont bodyFont() const

The markdown prose font (base 12 pt).

#### QFont codeFont() const

The code font (10.5 pt, fixed pitch), used for fenced blocks, inline code and the source editor.

#### QFont headingFont(int level) const

Heading font for `level` 1–6, clamped into range. Weight and size both step down with level.

#### qreal bodyPointSize() const / qreal codePointSize() const / static qreal headingPointSize(int level)

The underlying scale values, exposed for layout arithmetic.

## 11. Ownership and Lifecycle

Value type; no heap ownership. Font resources are owned by the application font database once registered.

## 12. Thread Safety

GUI-thread only. `QFontDatabase` registration is not meant to be called concurrently.

## 14. Inter-Class Interactions

- `MarkdownBuilder` reads `bodyFont()`, `codeFont()` and `headingFont()` through its `RenderStyle`.
- `MarkdownEditor` sets its widget font from `codeFont()`.
- `MarkdownView` keeps one in its `RenderStyle`.

## 16. Usage Example

```cpp
FontManager fonts(FontManager::FontSet::System);
editor->setFont(fonts.codeFont());
```