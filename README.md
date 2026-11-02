<p align="center"><img src="assets/icon/ChojemaInputLens-1024.png" width="112" alt="Chojema InputLens"></p>

<h1 align="center">Chojema InputLens</h1>

<p align="center">누른 키와 마우스 입력을 화면에 잠깐 보여 주는 가벼운 상주 앱</p>

<p align="center"><strong>한국어</strong> | <a href="README.en.md">English</a></p>

> 개발 초기 단계입니다. Windows x64와 macOS 12 이상(Apple Silicon·Intel)을 지원합니다.

## 다운로드

[최신 릴리스](https://github.com/chojema/ChojemaInputLens/releases/latest)에서 운영체제에 맞는 파일을 받습니다.

- **Windows:** `win-x64.zip`을 풀고 실행합니다. 설치와 관리자 권한은 필요 없습니다. 코드 서명이 없어 SmartScreen 경고가 나오면 **추가 정보 → 실행**을 선택합니다.
- **macOS:** `mac-universal.dmg`를 열고 앱을 **Applications**로 드래그합니다. ZIP도 제공합니다. Apple 배포용 서명·공증이 없어 실행이 차단되면 한 번 실행을 시도한 뒤 **시스템 설정 → 개인정보 보호 및 보안 → 그래도 열기**를 사용합니다.

macOS에서는 **시스템 설정 → 개인정보 보호 및 보안 → 입력 모니터링**에서 앱을 허용해야 합니다. 목록에 없으면 **+**로 응용 프로그램 폴더의 앱을 추가하고, 허용 후 앱을 종료해 다시 실행합니다. [macOS 설치 안내](src/mac/INSTALL.md)

## 표시되는 입력

| 입력 | 표시 |
|---|---|
| 조합 키 | `Ctrl + C`, `Alt + Tab`, macOS의 `Cmd + C` |
| 기능·이동·IME 키 | `F5`, `Esc`, `←`, `한/영` |
| 마우스 | 누른 버튼이나 휠 방향을 강조한 마우스 그림 |

수정키 없이 입력하는 글자, 숫자, 기호, 공백은 표시하지 않습니다.

## 개인정보

입력 내용을 파일에 저장하거나 네트워크로 전송하지 않습니다. 트레이 또는 메뉴 막대 메뉴의 **일시정지**를 켜면 입력 수신 자체를 중단합니다. 저장하는 설정은 사용자가 고른 표시 언어뿐입니다.

## 사용법

Windows에서는 `ChojemaInputLens.exe`를 실행하면 알림 영역에 아이콘이 나타납니다. macOS에서는 `Chojema InputLens.app`을 실행하면 메뉴 막대에 키보드 아이콘이 나타나며 Dock 아이콘은 없습니다. 아이콘을 클릭하면 일시정지, 마우스 표시, 언어, 정보, 종료 메뉴가 열립니다.

시스템의 표시 언어가 영어이면 영어로, 그 밖의 환경에서는 한국어로 시작합니다. **언어** 메뉴에서 바꿀 수 있습니다.

## 제한 사항

- 관리자 권한으로 실행한 창에 포커스가 있으면 입력이 표시되지 않을 수 있습니다.
- 독점 전체 화면 앱 위에는 표시되지 않을 수 있습니다.
- `Ctrl + Alt + Del`은 표시되지 않습니다.
- macOS의 암호 입력 등 보안 입력이 활성화된 동안에는 키보드 입력이 표시되지 않을 수 있습니다.
- macOS의 미디어·밝기 키는 표시하지 않으며, F1~F20으로 전달된 기능 키를 표시합니다.

## 빌드

Windows 10 이상, Visual Studio C++ 도구, CMake, Ninja가 필요합니다. 개발자 명령 프롬프트에서 실행합니다.

```
cmake --preset release
cmake --build --preset release
```

macOS에서는 Xcode Command Line Tools를 설치한 뒤 실행합니다. CMake 없이 macOS 12 이상용 Universal 앱을 빌드합니다.

```sh
bash src/mac/build.sh          # build/macos/Chojema InputLens.app
bash src/mac/build.sh package  # DMG와 ZIP도 생성
```

CMake로 빌드할 수도 있습니다.

```sh
cmake -S . -B build/mac -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build/mac
```

## 변경 내역

[CHANGELOG.md](CHANGELOG.md)

## 라이선스

[MIT](LICENSE) © 2026 Chulwoo Park
