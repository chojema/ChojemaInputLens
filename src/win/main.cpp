#include <windows.h>
#include <shellapi.h>

#include <string>

#include "key_labels.h"
#include "overlay.h"
#include "resource.h"
#include "version.h"

#ifdef INPUTLENS_METRICS
#include <cstdio>
#endif

namespace {

constexpr wchar_t kMainClass[] = L"ChojemaInputLensMain";
constexpr wchar_t kAppName[] = L"Chojema InputLens";
constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT kTrayId = 1;
constexpr ULONGLONG kMergeWindowMs = 700;
constexpr ULONGLONG kModifierTapMs = 1000;

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

    bool ctrl = false, alt = false, shift = false, win = false;
    UINT tapCandidate = 0;
    ULONGLONG tapStart = 0;

    std::wstring lastLabel;
    MouseGlyph lastGlyph = MouseGlyph::None;
    ULONGLONG lastTime = 0;
    int repeat = 0;
    int wheelSum = 0;

#ifdef INPUTLENS_METRICS
    // Counters only. Key identities and labels are never recorded.
    std::wstring metricsPath;
    unsigned long long inputs = 0, mouseDropped = 0, keyboardEvents = 0, shown = 0;
    long long handlerTicksTotal = 0, handlerTicksMax = 0;
#endif
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

bool* ModifierFlag(UINT vk) {
    switch (vk) {
    case VK_CONTROL: return &g.ctrl;
    case VK_MENU: return &g.alt;
    case VK_SHIFT: return &g.shift;
    case VK_LWIN:
    case VK_RWIN: return &g.win;
    default: return nullptr;
    }
}

const wchar_t* ModifierName(UINT vk) {
    switch (vk) {
    case VK_CONTROL: return L"Ctrl";
    case VK_MENU: return L"Alt";
    case VK_SHIFT: return L"Shift";
    default: return L"Win";
    }
}

// Key-up can be lost when the secure desktop takes over, such as on Win+L.
void ReconcileModifiers() {
    const auto released = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) == 0; };
    if (g.ctrl && released(VK_CONTROL)) g.ctrl = false;
    if (g.alt && released(VK_MENU)) g.alt = false;
    if (g.shift && released(VK_SHIFT)) g.shift = false;
    if (g.win && released(VK_LWIN) && released(VK_RWIN)) g.win = false;
}

std::wstring ModifierPrefix() {
    std::wstring prefix;
    if (g.ctrl) prefix += L"Ctrl + ";
    if (g.alt) prefix += L"Alt + ";
    if (g.shift) prefix += L"Shift + ";
    if (g.win) prefix += L"Win + ";
    return prefix;
}

void Present(const std::wstring& label, MouseGlyph glyph = MouseGlyph::None, int amount = 1) {
    const ULONGLONG now = GetTickCount64();
    if (label == g.lastLabel && glyph == g.lastGlyph && now - g.lastTime <= kMergeWindowMs) {
        g.repeat += amount;
    } else {
        g.repeat = amount;
    }
    g.lastLabel = label;
    g.lastGlyph = glyph;
    g.lastTime = now;
#ifdef INPUTLENS_METRICS
    ++g.shown;
#endif
    g.overlay.Show(label, glyph, g.repeat > 1 ? L" ×" + std::to_wstring(g.repeat) : std::wstring());
}

void HandleKeyboard(const RAWKEYBOARD& kb) {
    UINT vk = kb.VKey;
    const bool extended = (kb.Flags & RI_KEY_E0) != 0;
    const bool up = (kb.Flags & RI_KEY_BREAK) != 0;
    // 0xFF and extended Shift are synthetic scan codes emitted around other keys.
    if (vk == 0 || vk == 0xFF || (vk == VK_SHIFT && extended)) return;
    if (vk == VK_LCONTROL || vk == VK_RCONTROL) vk = VK_CONTROL;
    if (vk == VK_LMENU || vk == VK_RMENU) vk = VK_MENU;
    if (vk == VK_LSHIFT || vk == VK_RSHIFT) vk = VK_SHIFT;

    if (bool* flag = ModifierFlag(vk)) {
        if (!up) {
            if (*flag) return;  // Auto-repeat of a held modifier.
            const bool first = !(g.ctrl || g.alt || g.shift || g.win);
            *flag = true;
            g.tapCandidate = first ? vk : 0;
            g.tapStart = GetTickCount64();
        } else {
            *flag = false;
            // A modifier alone shows on release, so capitals typed with Shift stay silent.
            if (g.tapCandidate == vk && GetTickCount64() - g.tapStart <= kModifierTapMs) {
                Present(ModifierName(vk));
            }
            g.tapCandidate = 0;
        }
        return;
    }

    if (up) return;
    g.tapCandidate = 0;
    ReconcileModifiers();

    if (g.ctrl || g.alt || g.win) {
        Present(ModifierPrefix() + ComboKeyLabel(vk, extended));
    } else if (const wchar_t* label = StandaloneKeyLabel(vk, extended)) {
        Present(ModifierPrefix() + label);
    }
    // Anything else is ordinary typing. It is dropped here and never stored.
}

void HandleMouse(const RAWMOUSE& mouse) {
    const USHORT flags = mouse.usButtonFlags;
    g.tapCandidate = 0;
    ReconcileModifiers();
    const std::wstring prefix = ModifierPrefix();
    if (flags & RI_MOUSE_LEFT_BUTTON_DOWN) Present(prefix, MouseGlyph::Left);
    if (flags & RI_MOUSE_RIGHT_BUTTON_DOWN) Present(prefix, MouseGlyph::Right);
    if (flags & RI_MOUSE_MIDDLE_BUTTON_DOWN) Present(prefix, MouseGlyph::Middle);
    if (flags & RI_MOUSE_BUTTON_4_DOWN) Present(prefix, MouseGlyph::Back);
    if (flags & RI_MOUSE_BUTTON_5_DOWN) Present(prefix, MouseGlyph::Forward);
    if (flags & (RI_MOUSE_WHEEL | RI_MOUSE_HWHEEL)) {
        // Wheel resolution varies, so count notches once the accumulated delta reaches one.
        g.wheelSum += static_cast<SHORT>(mouse.usButtonData);
        const int notches = g.wheelSum / WHEEL_DELTA;
        if (notches != 0) {
            g.wheelSum -= notches * WHEEL_DELTA;
            const bool horizontal = (flags & RI_MOUSE_HWHEEL) != 0;
            const MouseGlyph glyph = horizontal ? (notches > 0 ? MouseGlyph::WheelRight : MouseGlyph::WheelLeft)
                                                : (notches > 0 ? MouseGlyph::WheelUp : MouseGlyph::WheelDown);
            Present(prefix, glyph, notches > 0 ? notches : -notches);
        }
    }
}

void HandleRawInput(HRAWINPUT handle) {
#ifdef INPUTLENS_METRICS
    LARGE_INTEGER begin, end;
    QueryPerformanceCounter(&begin);
    ++g.inputs;
#endif
    RAWINPUT raw;
    UINT size = sizeof(raw);
    if (GetRawInputData(handle, RID_INPUT, &raw, &size, sizeof(RAWINPUTHEADER)) != UINT(-1)) {
        if (raw.header.dwType == RIM_TYPEMOUSE) {
            // Raw Input cannot filter out motion, so it must leave as early as possible.
            if (raw.data.mouse.usButtonFlags != 0) {
                HandleMouse(raw.data.mouse);
            }
#ifdef INPUTLENS_METRICS
            else {
                ++g.mouseDropped;
            }
#endif
        } else if (raw.header.dwType == RIM_TYPEKEYBOARD) {
#ifdef INPUTLENS_METRICS
            ++g.keyboardEvents;
#endif
            HandleKeyboard(raw.data.keyboard);
        }
    }
#ifdef INPUTLENS_METRICS
    QueryPerformanceCounter(&end);
    const long long elapsed = end.QuadPart - begin.QuadPart;
    g.handlerTicksTotal += elapsed;
    if (elapsed > g.handlerTicksMax) g.handlerTicksMax = elapsed;
#endif
}

void RegisterInput() {
    RAWINPUTDEVICE devices[2]{};
    devices[0] = {0x01, 0x06, g.paused ? DWORD(RIDEV_REMOVE) : DWORD(RIDEV_INPUTSINK), g.paused ? nullptr : g.hwnd};
    const bool mouseOn = !g.paused && g.showMouse;
    devices[1] = {0x01, 0x02, mouseOn ? DWORD(RIDEV_INPUTSINK) : DWORD(RIDEV_REMOVE), mouseOn ? g.hwnd : nullptr};
    RegisterRawInputDevices(devices, 2, sizeof(RAWINPUTDEVICE));
    if (g.paused) {
        g.ctrl = g.alt = g.shift = g.win = false;
        g.tapCandidate = 0;
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

#ifdef INPUTLENS_METRICS
void WriteMetrics() {
    if (g.metricsPath.empty()) return;
    FILE* file = nullptr;
    if (_wfopen_s(&file, g.metricsPath.c_str(), L"w") != 0 || !file) return;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    const double toUs = 1e6 / static_cast<double>(frequency.QuadPart);
    fprintf(file,
            "{\"wm_input\":%llu,\"mouse_dropped\":%llu,\"keyboard_events\":%llu,\"presented\":%llu,"
            "\"renders\":%llu,\"first_render_ms\":%.1f,\"max_render_ms\":%.1f,"
            "\"handler_avg_us\":%.3f,\"handler_max_us\":%.1f}\n",
            g.inputs, g.mouseDropped, g.keyboardEvents, g.shown, g.overlay.RenderCount(),
            g.overlay.FirstRenderMs(), g.overlay.MaxRenderMs(),
            g.inputs ? g.handlerTicksTotal * toUs / static_cast<double>(g.inputs) : 0.0,
            g.handlerTicksMax * toUs);
    fclose(file);
}
#endif

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
#ifdef INPUTLENS_METRICS
        WriteMetrics();
#endif
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

#ifdef INPUTLENS_METRICS
    int argc = 0;
    if (LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc)) {
        if (argc >= 3 && wcscmp(argv[1], L"--metrics-file") == 0) g.metricsPath = argv[2];
        LocalFree(argv);
    }
#endif

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
