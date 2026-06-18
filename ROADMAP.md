<!-- ROADMAP.md (markdown) -->

# Roadmap

## Qt Project Setup

- [ ] Initialize Qt6 C++ project with CMakeLists.txt
- [ ] Configure CMake presets for Windows (MSVC), Linux (GCC/Clang), macOS (Clang)
- [ ] Embed application icon (existing `assets/logo/vertex-icon.png` / `.svg`)
- [ ] Set Windows manifest and version resource (replaces `build.rs` + `winres`)
- [ ] Set up `windeployqt` step in CMake for Windows distribution
- [ ] Set up `linuxdeployqt` / AppImage packaging for Steam Deck
- [ ] Set up `macdeployqt` for macOS `.app` bundle
- [ ] Set up installer (Inno Setup for Windows, or Qt Installer Framework)

## Main App — Port from Rust Archive

### Window & Tray
- [ ] System tray icon (`QSystemTrayIcon`) with icon from assets
- [ ] Tray context menu: Open, Settings, Exit
- [ ] Left-click tray icon to open/show main window
- [ ] Hide to tray on window close (override `closeEvent`, respect setting)
- [ ] Start minimized setting (launch with main window hidden, tray visible)
- [ ] Restore window to foreground when opened from tray

### Settings Panel
- [ ] Settings dialog / panel with live save on change
- [ ] Toggle: Use game overlay
- [ ] Toggle: Auto-detect game install directory from running process
- [ ] Text field: Install directory (disabled when auto-detect is on)
- [ ] Text field: Windows executable name (default `PathOfExile.exe`)
- [ ] Text field: Linux executable name (default `PathOfExile`)
- [ ] Toggle: Start minimized
- [ ] Toggle: Minimize to tray on close
- [ ] Toggle: Auto start on boot *(stub — mark coming soon)*

### Activity Log
- [ ] Scrolling timestamped log view in main window (auto-scroll to bottom)
- [ ] Log entry on app start
- [ ] Log entry when game window detected / lost
- [ ] Log entry when install directory is auto-detected

### Config Persistence
- [ ] Load/save config from TOML file alongside executable (port of `config.rs`)
- [ ] Fall back to current working directory for `cargo run` dev workflow
- [ ] Write default config file on first launch if not present

## Window Tracker — Port from Rust Archive

- [ ] Abstract `WindowTracker` interface (port of `tracker.rs`)
- [ ] Windows implementation: `FindWindowA` + `GetWindowRect` + `QueryFullProcessImageNameW`
- [ ] Linux implementation: X11 window tree walk by title (port of `LinuxTracker`)
- [ ] Auto-detect game install directory from process image path (Windows)
- [ ] Poll game window every 1 second; update log on state change

## Game Overlay — Port and Fix

The Rust prototype used a separate binary with stdin IPC because `eframe` (winit) and `egui_overlay` (GLFW) could not share a process event loop. In Qt both windows live in the same process with no IPC needed.

- [ ] Create overlay as a second `QWidget` in the same process (no child process)
- [ ] Window flags: `FramelessWindowHint | WindowStaysOnTopHint | Tool` (no taskbar entry)
- [ ] Transparent background: `WA_TranslucentBackground`
- [ ] Track game window rect; reposition/resize overlay to match each poll cycle
- [ ] Fall back to a large default rect when game window is not found
- [ ] Per-region hit-testing via `QWidget::setMask(QRegion)`:
  - Background / empty areas: transparent and click-through to game
  - UI widget areas (buttons, inputs): interactive, capture mouse normally
  - Update mask whenever overlay layout changes
- [ ] Show/hide overlay with game window detection (show when game found + overlay enabled, hide otherwise)
- [ ] Config changes propagate to overlay directly (same process, shared object — no IPC)

## Cross-Monitor DPI

- [ ] Verify Qt sets `DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2` automatically (no manual call needed)
- [ ] Port the cross-monitor drag stability test from `tests/dpi_drag_test.rs` to a Qt-native integration test or manual test checklist

## Future

- [ ] Auto start on boot (Windows: registry `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`; Linux: `.desktop` file in `~/.config/autostart`)
- [ ] Companion mode: web API only, no overlay, no game filesystem — works on any platform including those without a PoE install
- [ ] macOS support (overlay requires `NSWindow` level + `ignoresMouseEvents` for passthrough; Qt wraps this but needs testing with PoE Mac client)
- [ ] Game overlay interactive content beyond proof-of-concept text
