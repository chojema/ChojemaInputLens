#include <windows.h>
#include <shellapi.h>

#include <string>

#include "key_labels.h"
#include "overlay.h"
#include "resource.h"
#include "version.h"


namespace {

constexpr wchar_t kMainClass[] = L"ChojemaInputLensMain";
constexpr wchar_t kAppName[] = L"Chojema InputLens";
constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT kTrayId = 1;

enum MenuId : UINT { kMenuPause = 1, kMenuMouse, kMenuKorean, kMenuEnglish, kMenuAbout, kMenuExit };

constexpr wchar_t kSettingsKey[] = L"Software\\Chojema\\InputLens";

struct App {
    HINSTANCE instance = nullptr;
    HWND hwnd = nullptr;
    HICON icon = nullptr;
    UINT taskbarCreated = 0;
    Overlay overlay;
    bool paused = false;
    bool showMouse = true;
    bool english = false;

    inputlens::InputTracker tracker;
    int wheelSum = 0;
    int hwheelSum = 0;

} g;

const wchar_t* T(const wchar_t* korean, const wchar_t* english) { return g.english ? english : korean; }

// A saved choice wins. Otherwise English Windows gets English and everything else Korean.
bool LoadEnglish() {
    wchar_t value[8]{};
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER, kSettingsKey, L"Language", RRF_RT_REG_SZ, nullptr, value, &size) == ERROR_SUCCESS) {
        return wcscmp(value, L"en") == 0;
    }
    return PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_ENGLISH;
}

void SetLanguage(bool english) {
    g.english = english;
    SetKeyLabelsEnglish(english);
    const wchar_t* value = english ? L"en" : L"ko";
    RegSetKeyValueW(HKEY_CURRENT_USER, kSettingsKey, L"Language", REG_SZ, value,
                    static_cast<DWORD>((wcslen(value) + 1) * sizeof(wchar_t)));
}

void Present(const std::optional<inputlens::Display>& display) {
    if (!display) return;
    g.overlay.Show(DisplayPrefix(*display), display->glyph,
                   display->count > 1 ? L" ×" + std::to_wstring(display->count) : std::wstring());
}

// Key-up can be lost when the secure desktop takes over, such as on Win+L.
void ReconcileModifiers() {
    const uint8_t tracked = g.tracker.Modifiers();
    if (tracked != 0) g.tracker.Reconcile(HeldModifiers(tracked));
}

void HandleKeyboard(const RAWKEYBOARD& kb) {
    const auto event = TranslateKey(kb.VKey, (kb.Flags & RI_KEY_E0) != 0, (kb.Flags & RI_KEY_BREAK) != 0);
    if (!event) return;
    if (event->down && event->key > inputlens::Key::Meta) ReconcileModifiers();
    Present(g.tracker.OnKey(*event, GetTickCount64()));
}

void HandleMouse(const RAWMOUSE& mouse) {
    const USHORT flags = mouse.usButtonFlags;
    const ULONGLONG now = GetTickCount64();
    g.tracker.CancelTap();
    ReconcileModifiers();
    if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) Present(g.tracker.OnMouseButton(MouseGlyph::Left, now));
    if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) Present(g.tracker.OnMouseButton(MouseGlyph::Right, now));
    if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) Present(g.tracker.OnMouseButton(MouseGlyph::Middle, now));
    if (flags & RI_MOUSE_BUTTON_4_DOWN) Present(g.tracker.OnMouseButton(MouseGlyph::Back, now));
    if (flags & RI_MOUSE_BUTTON_5_DOWN) Present(g.tracker.OnMouseButton(MouseGlyph::Forward, now));
    if (flags & (RI_MOUSE_WHEEL | RI_MOUSE_HWHEEL)) {
        // Wheel resolution varies, so count notches once the accumulated delta reaches one.
        const bool horizontal = (flags & RI_MOUSE_HWHEEL) != 0;
        int& sum = horizontal ? g.hwheelSum : g.wheelSum;
        sum += static_cast<SHORT>(mouse.usButtonData);
        const int notches = sum / WHEEL_DELTA;
        sum -= notches * WHEEL_DELTA;
        Present(g.tracker.OnWheel(notches, horizontal, now));
    }
}

void HandleRawInput(HRAWINPUT handle) {
    RAWINPUT raw;
    UINT size = sizeof(raw);
    if (GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != UINT(-1)) {
        if (raw.header.dwType == RIM_TYPEMOUSE) {
            // Raw Input cannot filter out motion, so it must leave as early as possible.
            if (raw.data.mouse.usButtonFlags != 0) {
                HandleMouse(raw.data.mouse);
            }
        } else if (raw.header.dwType == RIM_TYPEKEYBOARD) {
            HandleKeyboard(raw.data.keyboard);
        }
    }
}

void RegisterInput() {
    RAWINPUTDEVICE devices[2]{};
    devices[0] = {0x01, 0x06, g.paused ? DWORD(RIDEV_REMOVE) : DWORD(RIDEV_INPUTSINK), g.paused ? nullptr : g.hwnd};
    const bool mouseOn = !g.paused && g.showMouse;
    devices[1] = {0x01, 0x02, mouseOn ? DWORD(RIDEV_INPUTSINK) : DWORD(RIDEV_REMOVE), mouseOn ? g.hwnd : nullptr};
    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
    if (g.paused) {
        g.tracker.ResetModifiers();
        g.overlay.Hide();
    }
}

void AddTrayIcon() {
    NOTIFYICONDATAW nid{sizeof(nid)};
    nid.hWnd = g.hwnd;
    nid.uID = kTrayId;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_APP_TRAY;
    nid.hIcon = g.icon;
    wcscpy_s(nid.szTip, kAppName);
    Shell_NotifyIconW(NIM_ADD, &nid);
    nid.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &nid);
}

void RemoveTrayIcon() {
    NOTIFYICONDATAW nid{sizeof(nid)};
    nid.hWnd = g.hwnd;
    nid.uID = kTrayId;
    Shell_NotifyIconW(NIM_DELETE, &nid);
}

void ShowAbout() {
    const std::wstring text = std::wstring(kAppName) + L" " + INPUTLENS_VERSION_WSTRING + L"\n\n" +
                              T(L"입력한 내용을 저장하거나 전송하지 않습니다.", L"Input is never stored or sent anywhere.") +
                              L"\n\nCopyright (c) 2026 Chulwoo Park";
    MessageBoxW(g.hwnd, text.c_str(), T(L"정보", L"About"), MB_OK | MB_ICONINFORMATION);
}

void ShowTrayMenu(POINT at) {
    HMENU languages = CreatePopupMenu();
    AppendMenuW(languages, MF_STRING | (g.english ? 0 : MF_CHECKED), kMenuKorean, L"한국어");
    AppendMenuW(languages, MF_STRING | (g.english ? MF_CHECKED : 0), kMenuEnglish, L"English");

    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING | (g.paused ? MF_CHECKED : 0), kMenuPause, T(L"일시정지", L"Pause"));
    AppendMenuW(menu, MF_STRING | (g.showMouse ? MF_CHECKED : 0), kMenuMouse, T(L"마우스 표시", L"Show mouse"));
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(languages), T(L"언어", L"Language"));
    AppendMenuW(menu, MF_STRING, kMenuAbout, T(L"정보", L"About"));
    AppendMenuW(menu, MF_STRING, kMenuExit, T(L"종료", L"Exit"));

    SetForegroundWindow(g.hwnd);  // Required so the menu closes on an outside click.
    const UINT id = TrackPopupMenuEx(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON, at.x, at.y, g.hwnd, nullptr);
    PostMessageW(g.hwnd, WM_NULL, 0, 0);
    DestroyMenu(menu);

    switch (id) {
    case kMenuPause:
        g.paused = !g.paused;
        RegisterInput();
        break;
    case kMenuMouse:
        g.showMouse = !g.showMouse;
        RegisterInput();
        break;
    case kMenuKorean:
        SetLanguage(false);
        break;
    case kMenuEnglish:
        SetLanguage(true);
        break;
    case kMenuAbout:
        ShowAbout();
        break;
    case kMenuExit:
        DestroyWindow(g.hwnd);
        break;
    default:
        break;
    }
}


LRESULT CALLBACK MainProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (g.taskbarCreated != 0 && msg == g.taskbarCreated) {
        AddTrayIcon();  // Explorer restarted.
        return 0;
    }
    switch (msg) {
    case WM_INPUT:
        HandleRawInput(reinterpret_cast<HRAWINPUT>(lp));
        return DefWindowProcW(hwnd, msg, wp, lp);
    case WM_APP_TRAY:
        if (LOWORD(lp) == WM_CONTEXTMENU || LOWORD(lp) == WM_LBUTTONUP) {
            POINT at;
            GetCursorPos(&at);
            ShowTrayMenu(at);
        }
        return 0;
    case WM_DESTROY:
        RemoveTrayIcon();
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    HANDLE single = CreateMutexW(nullptr, TRUE, L"Local\\ChojemaInputLens.Instance");
    if (!single || GetLastError() == ERROR_ALREADY_EXISTS) return 0;


    g.instance = instance;
    g.english = LoadEnglish();
    SetKeyLabelsEnglish(g.english);
    g.taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    g.icon = static_cast<HICON>(LoadImageW(instance, MAKEINTRESOURCEW(IDI_APP), IMAGE_ICON,
                                           GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = MainProc;
    wc.hInstance = instance;
    wc.lpszClassName = kMainClass;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_APP));
    if (!RegisterClassExW(&wc)) return 1;

    g.hwnd = CreateWindowExW(0, kMainClass, kAppName, WS_OVERLAPPED, 0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
    if (!g.hwnd || !g.overlay.Create(instance)) return 1;

    AddTrayIcon();
    RegisterInput();

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        DispatchMessageW(&msg);
    }

    g.overlay.Destroy();
    if (g.icon) DestroyIcon(g.icon);
    CloseHandle(single);
    return 0;
}
