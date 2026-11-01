# ScrollSync

## 1. Class Overview

`ScrollSync` keeps the vertical scroll positions of two scroll areas in step, so
the source pane and the preview pane show the same region of the document. The
link is proportional: the fraction scrolled in one pane is applied to the other.
This is deliberately not a line-by-line mapping — rendered markdown collapses
source lines, and a proportional link stays stable while the document is edited.
A re-entrancy guard stops the two views from bouncing the value back and forth.

## 2. Project Structure and Dependencies

- Instantiated by `app/MainWindow` with the editor and the preview.
- Depends on Qt6 Widgets (`QAbstractScrollArea`, `QScrollBar`).

## 3. Class Hierarchy and Role

Derives from `QObject`. It connects to both scrollbars' `valueChanged` signals
and pushes a proportional value to the other, guarded against recursion.

## 9. Public Methods

#### ScrollSync(QAbstractScrollArea *first, QAbstractScrollArea *second, QObject *parent = nullptr)

Wires the two areas in both directions. Neither area is owned; both must outlive
the synchroniser.

## 11. Ownership and Lifecycle

`QObject`; parent-owned by the window. It holds non-owning raw pointers to the
two scroll areas — a documented ownership boundary (`// not owned`): creating a
`ScrollSync` whose areas die first leaves dangling pointers.

## 12. Thread Safety

GUI-thread only: it writes scrollbar values.

## 14. Inter-Class Interactions

- `MainWindow` constructs it to link `MarkdownEditor` and `MarkdownView`.

## 16. Usage Example

```cpp
auto *sync = new ScrollSync(editor, view, window);
```