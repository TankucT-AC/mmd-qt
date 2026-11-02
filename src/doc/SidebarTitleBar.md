# SidebarTitleBar

## 1. Class Overview

`SidebarTitleBar` is the sidebar dock's own title bar: the title on the left, then
two buttons — float and close — sized to be hit with a pointer.

Qt's stock dock buttons are about 18x14 under Fusion, and a style sheet cannot
enlarge them (`QDockWidget::close-button { width: …; height: … }` is ignored), so
the sidebar draws its own.

Each button paints its own glyph rather than carrying a `QIcon` built from an
18 px pixmap: such a pixmap is a bitmap at that size, so a 2x screen stretches it
to 36 device pixels and it goes soft. Painting on the widget keeps the glyph in
device coordinates at whatever ratio the screen has — the same recipe as the code
box's copy button. The pen follows `QPalette::WindowText`, so a theme change
repaints correctly without any extra call.

Trade-off: replacing the title bar gives up Qt's drag-to-redock gesture. The
float button still detaches the pane, and a floating dock can be dragged back
into place by its own window frame.

## 2. Project Structure and Dependencies

- Instantiated by `app/MainWindow`, which hands it to `QDockWidget::setTitleBarWidget`.
- Depends on Qt6 Widgets only; the theme arrives as a `ThemeDefinition`.

## 3. Class Hierarchy and Role

Derives from `QWidget`. It is a plain title bar, not a dock: it asks the
`QDockWidget` it was built for to hide or float.

## 5. Enumerations

The icon glyphs are an internal enum (`Float`, `Close`) — nothing outside the
class needs to name them.

## 9. Public Methods

#### explicit SidebarTitleBar(QDockWidget *dock, const QString &title, QWidget *parent = nullptr)

Builds the label and the two buttons and wires them to the dock: close hides it,
float toggles `QDockWidget::setFloating`.

## 11. Ownership and Lifecycle

Parent-owned by the dock widget, which deletes it with itself. The buttons are
children of the title bar.

## 12. Thread Safety

GUI-thread only.

## 14. Inter-Class Interactions

- `MainWindow::buildSidebar` creates it and installs it with `setTitleBarWidget`.
- The buttons paint from the widget palette, so `MainWindow::applyTheme` only has
  to set the application palette; there is no extra call.
- Test hooks: the buttons carry the object names `sidebarFloatButton` and
  `sidebarCloseButton`.

## 16. Usage Example

```cpp
auto *dock = new QDockWidget(tr("Sidebar"), window);
dock->setObjectName(QStringLiteral("sidebar"));
auto *titleBar = new SidebarTitleBar(dock, tr("Sidebar"), dock);
dock->setTitleBarWidget(titleBar);
```
