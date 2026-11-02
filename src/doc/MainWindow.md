# MainWindow

## 1. Class Overview

`MainWindow` is the composition root: it builds the two panes, the menus, and the
wiring between the data layer and the views. It owns the theme manager, the file
document and the two panes, and it holds the policies that belong to the window
rather than to any reusable class — the unsaved-changes guard, the
external-change prompt, and the title's modified marker.

## 2. Project Structure and Dependencies

- Entry point (`src/main.cpp`) creates it.
- Depends on Qt6 Widgets (`QMainWindow`, menus, dialogs), `core/ThemeManager`, `core/FontManager`, `core/FileDocument`, `editor/MarkdownEditor`, `viewer/MarkdownView`, `app/FontsDialog`.

## 3. Class Hierarchy and Role

Derives from `QMainWindow`.

## 9. Public Methods

#### explicit MainWindow(QWidget *parent = nullptr)

Builds the UI and menus, wires signals, applies the initial theme and title.

#### QWidget *editorPanel() const / QWidget *viewerPanel() const

The two pane widgets, for tests and for focus handling.

#### QAction *toggleEditorAction() const / QAction *toggleViewerAction() const

The checkable actions that show/hide each pane (`F9` / `F10`). The shipped
default is a viewer-only window; the last layout is kept in `QSettings`
(`view/editorVisible`, `view/viewerVisible`) and restored on the next start.

**View → Load Remote Images** controls whether the preview downloads `http(s)`
images. It is on by default; the choice is stored under `view/remoteImages`
and applied through `MarkdownView::setRemoteImagesEnabled`. Turning it off
leaves the viewer entirely off the network and renders such references as their
alt-text chip.

#### void applyFontChoice(const QString &bodyFamily, const QString &codeFamily)

Typesets the preview and the editor with the chosen families and stores them
under `fonts/body` and `fonts/code`. Blank families mean the bundled faces, so
a blank pair resets; `FontsDialog` funnels here on accept, and
`restoreFontSettings` replays the stored choice at startup.

#### ThemeManager *themeManager() const / MarkdownEditor *editor() const / MarkdownView *viewer() const / FileDocument *fileDocument() const

Access to the owned collaborators, primarily for tests and `main`.

#### bool openFile(const QString &path)

Loads a document by path, applies it to the editor and preview and updates the
title. Returns `false` and shows a message on failure.

#### bool saveFile()

Saves the current document, prompting for a path when untitled. Returns `false`
on failure or when the user cancels.

## 10. Protected Virtual Methods / Event Handlers

#### void closeEvent(QCloseEvent *event) [override]

Base: `QMainWindow`. Runs the unsaved-changes guard; closes only if the user
saves or discards.

## 11. Ownership and Lifecycle

Top-level widget. It parents the theme manager, file document, editor and
viewer, all destroyed with it.

## 12. Thread Safety

GUI-thread only.

## 14. Inter-Class Interactions

- Connects `MarkdownEditor::textChanged` → `MarkdownView::setMarkdown`.
- Connects `MarkdownView::sourceLineActivated` → `MarkdownEditor::gotoSourceLine`: a
  click in the preview puts the editor's caret on the line that block came from.
  The two panes scroll independently — a proportional scroll sync disagrees with
  the reader because rendered and source heights do not line up.
- Connects `OutlinePanel::entryActivated` → `activateOutlineEntry`, which scrolls
  the preview and (when the renderer and the source scan agree) the editor.
  The clicked row then pins the outline bar: `visibleHeadingChanged` is ignored
  until `MarkdownView::userScrolled` reports real reader movement
  (`releaseOutlinePin`), so a jump near the document end — where the scroll
  clamps and the heading cannot reach the reading line — leaves the bar on the
  clicked row instead of sliding to the last heading on screen.
- Connects `ThemeManager::themeChanged` → `applyTheme` (palette, sidebar hairline,
  editor, viewer).
- Connects `FileDocument::externallyChanged` → `onExternalChange`.
- Connects the editor document's `modificationChanged` → title update.

## 16. Usage Example

```cpp
MainWindow window;
window.show();
if (!path.isEmpty())
    window.openFile(path);
```