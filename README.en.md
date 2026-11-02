<p align="center"><img src="assets/icon/ChojemaInputLens-1024.png" width="112" alt="Chojema InputLens"></p>

<h1 align="center">Chojema InputLens</h1>

<p align="center">A lightweight tray app that briefly shows your key and mouse input on screen</p>

<p align="center"><a href="README.md">한국어</a> | <strong>English</strong></p>

> Early development. Supports Windows x64 and macOS 12 or later on Apple Silicon and Intel.

## Download

Get the file for your operating system from the [latest release](https://github.com/chojema/ChojemaInputLens/releases/latest).

- **Windows:** Extract `win-x64.zip` and run the app. No installation or administrator rights are needed. If SmartScreen warns about the unsigned app, choose **More info → Run anyway**.
- **macOS:** Open `mac-universal.dmg` and drag the app to **Applications**. A ZIP is also available. The app has no Apple distribution signature or notarization. If macOS blocks it, attempt to launch it once, then use **System Settings → Privacy & Security → Open Anyway**.

On macOS, allow the app under **System Settings → Privacy & Security → Input Monitoring**. If it is missing, use **+** to add the app from Applications. Quit and reopen the app after granting permission. [macOS installation guide](src/mac/INSTALL.md)

## What it shows

| Input | Display |
|---|---|
| Key combinations | `Ctrl + C`, `Alt + Tab`, `Cmd + C` on macOS |
| Function, navigation and IME keys | `F5`, `Esc`, `←`, `Han/Eng` |
| Mouse | A mouse image highlighting the button or wheel direction |

Letters, digits, symbols and spaces typed without a modifier are not shown.

## Privacy

Input is never written to a file or sent over the network. **Pause** in the tray or menu bar menu stops input reception entirely. The only saved setting is the display language you choose.

## Usage

On Windows, run `ChojemaInputLens.exe` for a notification area icon. On macOS, run `Chojema InputLens.app` for a keyboard icon in the menu bar, with no Dock icon. Click it for Pause, Show mouse, Language, About and Quit.

The app uses English when the system display language is English and Korean otherwise. You can change this under **Language**.

## Limitations

- Input may not appear while a window running as administrator has focus.
- The overlay may not appear over exclusive full-screen apps.
- `Ctrl + Alt + Del` is not shown.
- macOS keyboard events may be unavailable during Secure Input, such as password entry.
- macOS media and brightness keys are not shown; function keys received as F1–F20 are shown.

## Build

Requires Windows 10 or later, Visual Studio C++ tools, CMake and Ninja. Run from a Developer Command Prompt.

```
cmake --preset release
cmake --build --preset release
```

On macOS, install Xcode Command Line Tools and run the following. CMake is not required. The app is Universal and targets macOS 12 or later.

```sh
bash src/mac/build.sh          # build/macos/Chojema InputLens.app
bash src/mac/build.sh package  # also creates DMG and ZIP files
```

Alternatively, use CMake:

```sh
cmake -S . -B build/mac -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build/mac
```

## Changelog

[CHANGELOG.en.md](CHANGELOG.en.md)

## License

[MIT](LICENSE) © 2026 Chulwoo Park
