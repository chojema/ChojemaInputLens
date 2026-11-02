# Changelog

[한국어](CHANGELOG.md) | **English**

This project follows [Semantic Versioning](https://semver.org/). The version is set in `project(... VERSION ...)` in `CMakeLists.txt`, and release tags look like `v0.1.0`.

## [0.1.0] - 2026-10-31

### macOS distribution added - 2026-11-02

- Universal app for Apple Silicon and Intel Macs running macOS 12 or later.
- Menu bar controls, Input Monitoring permission guidance, Cmd and Option shortcuts, mouse images and a click-through overlay.
- Korean and English interfaces, pause, mouse visibility toggle and automatic hiding.
- DMG and ZIP distribution without Apple distribution signing or notarization.

### Added

- Briefly shows key combinations, function keys and mouse input on Windows.
- Mouse buttons and wheel input appear as a mouse image with the relevant part highlighted.
- Text typed without a modifier is not shown.
- The tray menu can pause the app and turn the mouse display on or off.
- Korean and English interfaces, with a language choice in the tray menu.
