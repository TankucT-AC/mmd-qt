# ThemeManager

## 1. Class Overview

`ThemeManager` turns a *mode* — Light, Dark, System or Custom — into the concrete
`ThemeDefinition` the application renders with, and tells the rest of the app
whenever that resolution changes.

The interesting mode is **System**: the manager follows
`QStyleHints::colorScheme()` and reacts to platform changes live, so flipping the
desktop between light and dark re-tints a running window without a restart.
Consumers never poll; they connect to `themeChanged`.

## 2. Project Structure and Dependencies

- Instantiated by `app/MainWindow`.
- Depends on Qt6 Gui (`QGuiApplication`, `QStyleHints`) and on `core/Theme`.
- No build-time dependency beyond Qt.

## 3. Class Hierarchy and Role

Derives from `QObject` for the meta-object system, signals/slots and
parent-based ownership. `MainWindow` creates it with itself as parent.

## 5. Enumerations

`Mode` is registered with `Q_ENUM`.

| Value | Description |
|-------|-------------|
| `ThemeManager::Mode::Light` | Always resolves to `ThemeDefinition::light()`, regardless of the platform. |
| `ThemeManager::Mode::Dark` | Always resolves to `ThemeDefinition::dark()`. |
| `ThemeManager::Mode::System` | Resolves from `QStyleHints::colorScheme()`; re-resolves on `colorSchemeChanged`. |
| `ThemeManager::Mode::Custom` | Resolves to the theme set through `setCustomTheme()`. |

## 7. Signals

#### void themeChanged(const ThemeDefinition &theme)

Emitted whenever the resolved theme changes: on a mode change, on a platform
colour-scheme change while in System mode, or when the custom theme is replaced
while in Custom mode. A slot should re-apply the palette and re-render. Setting
the same mode again does not re-emit.

## 9. Public Methods

#### void setMode(Mode mode)

Switches the active mode and emits `themeChanged` when the resolved theme
differs (a no-op when the mode is unchanged). Safe to call at any time.

#### Mode mode() const

Returns the current mode. A freshly constructed manager is in `System`.

#### void setCustomTheme(const ThemeDefinition &theme)

Stores the theme used in Custom mode. Emits `themeChanged` immediately if Custom
mode is already active.

#### ThemeDefinition currentTheme() const

Returns the currently resolved theme. Call it after construction to seed the
initial palette.

## 11. Ownership and Lifecycle

`QObject` with `parent` ownership; `MainWindow` passes itself, so the manager is
destroyed with the window. The internal `Custom` theme is stored by value.

## 12. Thread Safety

GUI-thread only. It reads `QGuiApplication::styleHints()` and connects to its
signal, both of which belong to the GUI thread.

## 13. QML Exposure

None — the project is Widgets-only.

## 14. Inter-Class Interactions

- `MainWindow` connects `ThemeManager::themeChanged` to `MainWindow::applyTheme`.
- `MainWindow` drives the mode from the View → Theme menu and from a loaded theme file.

## 16. Usage Example

```cpp
auto *themes = new ThemeManager(parent);
QObject::connect(themes, &ThemeManager::themeChanged, window, &MainWindow::applyTheme);

QApplication::setPalette(themes->currentTheme().palette);
themes->setMode(ThemeManager::Mode::System); // follows the desktop
```