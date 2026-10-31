# 변경 내역(Changelog)

[유의적 버전](https://semver.org/lang/ko/)을 따릅니다. 버전은 `CMakeLists.txt`의 `project(... VERSION ...)`에서 관리하며, 릴리스 태그는 `v0.1.0` 형식입니다.
This project follows [Semantic Versioning](https://semver.org/). The version is set in `project(... VERSION ...)` in `CMakeLists.txt`, and release tags look like `v0.1.0`.

## [0.1.0] - 미출시(Unreleased)

### 추가(Added)

- Windows에서 조합 키, 기능 키, 마우스 입력을 화면에 잠깐 표시합니다.
  Briefly shows key combinations, function keys and mouse input on Windows.
- 마우스 버튼과 휠 입력은 해당 부분을 강조한 마우스 그림으로 표시합니다.
  Mouse buttons and wheel input appear as a mouse image with the relevant part highlighted.
- 수정키 없이 입력하는 글자는 표시하지 않습니다.
  Text typed without a modifier is not shown.
- 트레이 메뉴에서 일시정지와 마우스 표시 여부를 전환할 수 있습니다.
  The tray menu can pause the app and turn the mouse display on or off.
