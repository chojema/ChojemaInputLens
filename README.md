<p align="center"><img src="assets/icon/ChojemaInputLens-1024.png" width="112" alt="Chojema InputLens"></p>

<h1 align="center">Chojema InputLens</h1>

<p align="center">누른 키와 마우스 입력을 화면에 잠깐 보여 주는 가벼운 상주 앱</p>

<p align="center"><strong>한국어</strong> | <a href="README.en.md">English</a></p>

> 개발 초기 단계이며 아직 공개 릴리스가 없습니다. 현재 Windows 버전만 있습니다.

## 표시되는 입력

| 입력 | 표시 |
|---|---|
| 조합 키 | `Ctrl + C`, `Alt + Tab` |
| 기능·이동·IME 키 | `F5`, `Esc`, `←`, `한/영` |
| 마우스 | 누른 버튼이나 휠 방향을 강조한 마우스 그림 |

수정키 없이 입력하는 글자, 숫자, 기호, 공백은 표시하지 않습니다.

## 개인정보

입력 내용을 파일에 저장하거나 네트워크로 전송하지 않습니다. 트레이 메뉴의 **일시정지**를 켜면 입력 수신 자체를 중단합니다. 저장하는 설정은 사용자가 고른 표시 언어뿐입니다.

## 사용법

`ChojemaInputLens.exe`를 실행하면 알림 영역에 아이콘이 나타납니다. 아이콘을 클릭하면 일시정지, 마우스 표시, 언어, 정보, 종료 메뉴가 열립니다.

영어 Windows에서는 영어로, 그 밖의 환경에서는 한국어로 시작합니다. **언어** 메뉴에서 바꿀 수 있습니다.

## 제한 사항

- 관리자 권한으로 실행한 창에 포커스가 있으면 입력이 표시되지 않을 수 있습니다.
- 독점 전체 화면 앱 위에는 표시되지 않을 수 있습니다.
- `Ctrl + Alt + Del`은 표시되지 않습니다.

## 빌드

Windows 10 이상, Visual Studio C++ 도구, CMake, Ninja가 필요합니다. 개발자 명령 프롬프트에서 실행합니다.

```
cmake --preset release
cmake --build --preset release
```

## 변경 내역

[CHANGELOG.md](CHANGELOG.md)

## 라이선스

[MIT](LICENSE) © 2026 Chulwoo Park
