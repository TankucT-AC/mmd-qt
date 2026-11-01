# MainWindow

## 1. Class Overview

`MainWindow` is the composition root: it builds the two panes, the menus, and the
wiring between the data layer and the views. It owns the theme manager, the file
document, the scroll synchroniser and the two panes, and it holds the policies
that belong to the window rather than to any reusable class — the unsaved-changes
guard, the external-change prompt, and the title's modified marker.

## 2. Project Structure and Dependencies

- Entry point (`src/main.cpp`) creates it.
- Depends on Qt6 Widgets (`QMainWindow`, menus, dialogs), `core/ThemeManager`, `core/FileDocument`, `editor/MarkdownEditor`, `viewer/MarkdownView`, `app/ScrollSync`.

## 3. Class Hierarchy and Role

Derives from `QMainWindow`.

## 9. Public Methods

#### explicit MainWindow(QWidget *parent = nullptr)

Builds the UI and menus, wires signals, applies the initial theme and title.

#### QWidget *editorPanel() const / QWidget *viewerPanel() const

The two pane widgets, for tests and for focus handling.

#### QAction *toggleEditorAction() const / QAction *toggleViewerAction() const

The checkable actions that show/hide each pane (`F9` / `F10`).

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

Top-level widget. It parents the theme manager, file document, scroll sync,
editor and viewer, all destroyed with it.

## 12. Thread Safety

GUI-thread only.

## 14. Inter-Class Interactions

- Connects `MarkdownEditor::textChanged` → `MarkdownView::setMarkdown`.
- Connects `ThemeManager::themeChanged` → `applyTheme` (palette + editor + viewer).
- Connects `FileDocument::externallyChanged` → `onExternalChange`.
- Connects the editor document's `modificationChanged` → title update.

## 16. Usage Example

```cpp
MainWindow window;
window.show();
if (!path.isEmpty())
    window.openFile(path);
```