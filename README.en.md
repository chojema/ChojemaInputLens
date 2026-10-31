<p align="center"><img src="assets/icon/ChojemaInputLens-1024.png" width="112" alt="Chojema InputLens"></p>

<h1 align="center">Chojema InputLens</h1>

<p align="center">A lightweight tray app that briefly shows your key and mouse input on screen</p>

<p align="center"><a href="README.md">한국어</a> | <strong>English</strong></p>

> Early development. No public release yet. Only the Windows version exists for now.

## What it shows

| Input | Display |
|---|---|
| Key combinations | `Ctrl + C`, `Alt + Tab` |
| Function, navigation and IME keys | `F5`, `Esc`, `←`, `Han/Eng` |
| Mouse | A mouse image highlighting the button or wheel direction |

Letters, digits, symbols and spaces typed without a modifier are not shown.

## Privacy

Input is never written to a file or sent over the network. **Pause** in the tray menu stops input reception entirely. The only saved setting is the display language you choose.

## Usage

Run `ChojemaInputLens.exe` and an icon appears in the notification area. Click it for Pause, Show mouse, Language, About and Exit.

The app uses English on English Windows and Korean otherwise. You can change this under **Language**.

## Limitations

- Input may not appear while a window running as administrator has focus.
- The overlay may not appear over exclusive full-screen apps.
- `Ctrl + Alt + Del` is not shown.

## Build

Requires Windows 10 or later, Visual Studio C++ tools, CMake and Ninja. Run from a Developer Command Prompt.

```
cmake --preset release
cmake --build --preset release
```

## Changelog

[CHANGELOG.en.md](CHANGELOG.en.md)

## License

[MIT](LICENSE) © 2026 Chulwoo Park
