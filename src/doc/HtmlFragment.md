# HtmlFragment

## 1. Class Overview

`parseHtmlFragment` is a tiny, tolerant HTML tokenizer. Qt's XML reader is too
strict for the HTML that shows up in real markdown — unclosed tags, attributes
without quotes, raw text — so it scans the handful of constructs a README
actually uses: start tags, end tags, void elements, comments and declarations.
Malformed input is never fatal: an unterminated tag simply becomes text.

`MarkdownBuilder` uses it twice: for the inline HTML a markdown span carries,
and for the raw HTML md4c hands over as a block. Each token records where it
sits in the source, which is how the block renderer extracts the raw inner
markup (markdown and all) of a container such as a `<td>`.

## 2. Project Structure and Dependencies

- Used by `core/MarkdownBuilder` for `MD_SPAN_HTML` and `MD_BLOCK_HTML`.
- Depends only on Qt6 Core (`QString`, `QVector`).

## 3. Class Hierarchy and Role

Free functions, no classes and no state. `HtmlToken` is a plain value type.

## 4. Types

#### struct HtmlToken

| Field | Meaning |
|-------|---------|
| `kind` | `Text`, `StartTag`, `EndTag`, `SelfClosingTag`, `Comment` or `Declaration`. |
| `name` | Lowercased tag name (empty for text and comments). |
| `attributes` | Lowercased name/value pairs, in source order. |
| `text` | Content of a text or comment token. |
| `selfClosing` | True for `<name/>` and for HTML void elements (`<br>`, `<img>`). |
| `start` / `end` | Half-open offset of the token in the source string; slicing `source.mid(start, end - start)` returns the token's own text. |

## 9. Public Functions

#### QVector<HtmlToken> parseHtmlFragment(const QString &html)

Splits a raw HTML fragment into tokens. Never throws; a stray `<` with no tag
becomes a text token, and an unclosed tag runs to the end of the input.

#### QString htmlAttribute(const HtmlToken &token, const QString &name)

The value of an attribute, or an empty string when it is absent.

#### bool isHtmlVoidElement(const QString &name)

True for elements that never have a closing tag (`img`, `br`, `hr`, …).

## 10. Internal State and Algorithms

A single forward scan. Text between tags is emitted as a `Text` token; `<` that
does not begin a comment, declaration or tag name is emitted as a literal `<`.
Attribute values accept double quotes, single quotes or none. Offsets are
recorded for every token so a caller can recover the exact source slice.

## 11. Ownership and Lifecycle

No ownership; every function is pure and returns value types.

## 12. Thread Safety

Stateless and thread-safe.

## 14. Inter-Class Interactions

- `MarkdownBuilder::applyInlineHtml` walks the tokens for span-level HTML.
- `MarkdownBuilder::renderHtmlFragment` / `renderHtmlContainer` walk them for
  block-level HTML and use the token offsets to slice container interiors;
  `MarkdownBuilder::renderHtmlTable` slices cell content the same way.

## 16. Usage Example

```cpp
const QVector<HtmlToken> tokens = parseHtmlFragment(QStringLiteral("<td width=\"50%\">hi</td>"));
// tokens[0] is the <td> start tag; source.mid(tokens[0].start, tokens[0].end - tokens[0].start)
// is exactly "<td width=\"50%\">".
```