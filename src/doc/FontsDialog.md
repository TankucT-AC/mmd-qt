# FontsDialog

## 1. Class Overview

`FontsDialog` is the `View → Fonts…` dialog: two font combo boxes — the text
face for prose and headings, and a monospace face for code blocks and the
source editor — plus a button that puts the bundled faces back into the combos.
It only *collects* the choice; applying and remembering it belongs to
`MainWindow::applyFontChoice`, which the dialog funnels into on accept.

## 2. Project Structure and Dependencies

- Lives in `src/app/FontsDialog.{h,cpp}`; opened by `MainWindow::chooseFonts`.
- Depends on Qt6 Widgets (`QDialog`, `QFontComboBox`) and `core/FontManager`
  (to make sure the bundled faces are registered and therefore selectable).

## 3. Class Hierarchy and Role

Derives from `QDialog`. A plain collector: no connection to the documents, no
settings access.

## 9. Public Methods

#### explicit FontsDialog(QWidget *parent = nullptr)

Builds the form. The monospace combo lists only fixed-pitch families; the text
combo lists everything installed, including the bundled Inter and JetBrains
Mono, which are registered application fonts.

#### void setBodyFamily(const QString &family) / QString bodyFamily() const

Prefills or reports the text face.

#### void setCodeFamily(const QString &family) / QString codeFamily() const

Prefills or reports the monospace face.

## 11. Ownership and Lifecycle

Stack-allocated by `chooseFonts` and destroyed on return; `exec()` makes the
choice modal.

## 12. Thread Safety

GUI-thread only.

## 14. Inter-Class Interactions

- `MainWindow::chooseFonts` prefills it with the current faces (`QFontInfo` of
  the active `RenderStyle` fonts) and calls `applyFontChoice` with the result.
- The reset button carries the object name `fontsResetButton` (test hook) and
  restores `Inter` / `JetBrains Mono` in the combos.

## 16. Usage Example

```cpp
FontsDialog dialog(window);
dialog.setBodyFamily(QStringLiteral("Inter"));
if (dialog.exec() == QDialog::Accepted)
    window->applyFontChoice(dialog.bodyFamily(), dialog.codeFamily());
```
