# XLaunch native launcher — macOS and Linux/X11

Qt 6.8+ / Qt Quick / C++17. macOS app discovery and activation use AppKit. Linux/X11 discovery reads freedesktop `.desktop` entries, loads the active icon theme, and launches apps through `gio`; the UI and filtering model are portable.

The user's selected stack for both XLaunch and XDock is C++ + Qt Quick/QML. The current console under `dashboard/` is legacy code and is not the launcher UI.

Themes: default Amber Classic, or “跟随系统主题” in the menu-bar menu (also `--system-theme`). The system mode uses the Qt platform palette and default font, persisted across restarts. This implements palette/font integration, not pixel-identical native macOS widgets or direct parsing of GTK CSS. Linux GTK palette/font integration requires a compatible Qt platform-theme plugin and remains to be validated on GNOME/Xfce; KDE/DDE adapters likewise remain future work.

## Build on Linux/X11

```sh
sudo apt install qt6-base-dev qt6-declarative-dev qt6-tools-dev cmake g++ libglib2.0-bin
cmake -S launcher -B build/launcher -DCMAKE_BUILD_TYPE=Release
cmake --build build/launcher --parallel
QT_QPA_PLATFORM=xcb ctest --test-dir build/launcher --output-on-failure
QT_QPA_PLATFORM=xcb ./build/launcher/xlaunch
```

The launcher uses the current X11 desktop session and does not require Wayland or a compositor-specific protocol.

For a headless VPS or container, set `DISPLAY` to an Xvfb display and keep
`QT_QPA_PLATFORM=xcb`. Xvfb is CPU-rendered and has no GPU requirement; use
X11VNC/noVNC as a separate display transport when remote access is needed.

The default window is a compact `Menu` mode with category shortcuts. `全部应用` or Enter switches to the full-screen app launcher; the `Menu` button or Esc returns to the compact menu. In the ICEWM/XDock profile, compact Menu is positioned at the lower-left of the available work area so its bottom edge meets the top of the XDock strut; full-screen mode remains screen-wide.

For the combined ICEWM desktop shell, run `scripts/start-icewm-ai-desktop.sh` from the ICEWM session startup. It starts XDock and XLaunch with `QT_QPA_PLATFORM=xcb`; the existing Console/API services remain user services.

## Build on macOS

```sh
brew install qtbase qtdeclarative cmake
cmake -S launcher -B build/launcher -DCMAKE_PREFIX_PATH="$(brew --prefix)" -DCMAKE_BUILD_TYPE=Release
cmake --build build/launcher -j 6
ctest --test-dir build/launcher --output-on-failure
open build/launcher/XLaunch.app
```

Use Qt's `macdeployqt` with `-qmldir=launcher/qml` to bundle runtime dependencies before distributing. A production release requires Developer ID signing and notarization. The local build is a development app, not a notarized release. Follow Qt's applicable redistribution/license requirements when shipping its libraries.

## Implemented

- Real app discovery in `/Applications`, `~/Applications`, `/System/Applications`, and system CoreServices Applications. Bundle recursion is pruned to avoid internal helpers.
- Native app icons loaded on demand, cached for the session; GPU-rendered QML grid only instantiates visible delegates.
- Case-insensitive name filtering, category filtering, refresh from the menu-bar menu.
- Mouse and keyboard activation; arrow navigation, Enter launch, Escape clear/hide, Command-F search.
- Native LaunchServices activation, without executing interpolated shell commands.
- Menu-bar reopen/quit and Dock activation reopen, amber background and responsive scrollable grid.
- Classic Launchpad-inspired horizontal pages, clickable page dots, arrow-key selection and Control-Left/Right page navigation. Reopening resets search/category. Icon dimensions remain 76 logical design pixels scaled with the viewport; tighter cells and surrounding margins create density without shrinking icons.

This borrows pre-macOS-15 Launchpad's browse/search/launch pattern while retaining the chosen category rail. Folder grouping, drag-to-reorder and edit/jiggle mode are not yet implemented.

## Current limits

This is an independent launcher you can put in the Dock. It does not patch or delete Apple's launcher or replace a reserved system shortcut. Global shortcut registration, login startup, user-defined categories, real-time wallpaper sampling, live application-change monitoring, Agent integration and system-action providers remain unimplemented. AI is explicitly unavailable in the UI; local app launching works without an AI service.

Category metadata is best effort; uncategorized apps appear under Utilities. The bundled amber wallpaper is a clean ImageGen asset derived from the selected design. Installed macOS app icons/content naturally differ from the Linux concept screenshot. No user app inventory is sent to a server.

## Architecture

`src/catalog.*`: shared filtering and activation model. `qml/Main.qml`: shared presentation. `src/platform_macos.mm`: macOS discovery/icons/activation. `src/platform_linux.cpp`: freedesktop application discovery, icon-theme lookup and `gio` activation. `src/platform_stub.cpp`: fallback adapter for unsupported platforms. XDock remains an independent repository and is not required.

Qt Quick uses a scene graph with Metal on macOS and graphics backends on other platforms: [official scene graph documentation](https://doc.qt.io/qt-6/qtquick-visualcanvas-scenegraph.html). This choice provides portable UI rendering, not automatic support for every desktop's system integration.
