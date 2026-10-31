#include "key_labels.h"

namespace {
bool g_english = false;
}

void SetKeyLabelsEnglish(bool english) { g_english = english; }

const wchar_t* StandaloneKeyLabel(UINT vk, bool extended) {
    static const wchar_t* const kFunctionKeys[] = {
        L"F1",  L"F2",  L"F3",  L"F4",  L"F5",  L"F6",  L"F7",  L"F8",
        L"F9",  L"F10", L"F11", L"F12", L"F13", L"F14", L"F15", L"F16",
        L"F17", L"F18", L"F19", L"F20", L"F21", L"F22", L"F23", L"F24"};
    if (vk >= VK_F1 && vk <= VK_F24) return kFunctionKeys[vk - VK_F1];

    // Without the extended flag these are numeric keypad keys, which can type digits.
    if (extended) {
        switch (vk) {
        case VK_INSERT: return L"Insert";
        case VK_DELETE: return L"Delete";
        case VK_HOME: return L"Home";
        case VK_END: return L"End";
        case VK_PRIOR: return L"Page Up";
        case VK_NEXT: return L"Page Down";
        case VK_LEFT: return L"←";
        case VK_UP: return L"↑";
        case VK_RIGHT: return L"→";
        case VK_DOWN: return L"↓";
        default: break;
        }
    }
    switch (vk) {
    case VK_ESCAPE: return L"Esc";
    case VK_SNAPSHOT: return L"Print Screen";
    case VK_PAUSE: return L"Pause";
    case VK_APPS: return L"Menu";
    case VK_CAPITAL: return L"Caps Lock";
    case VK_NUMLOCK: return L"Num Lock";
    case VK_SCROLL: return L"Scroll Lock";
    case VK_HANGUL: return g_english ? L"Han/Eng" : L"한/영";
    case VK_HANJA: return g_english ? L"Hanja" : L"한자";
    default: return nullptr;
    }
}

std::wstring ComboKeyLabel(UINT vk, bool extended) {
    if (const wchar_t* label = StandaloneKeyLabel(vk, extended)) return label;
    switch (vk) {
    case VK_SPACE: return L"Space";
    case VK_RETURN: return L"Enter";
    case VK_TAB: return L"Tab";
    case VK_BACK: return L"Backspace";
    case VK_INSERT: return L"Num 0";
    case VK_DELETE: return L"Num .";
    default: break;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) return L"Num " + std::to_wstring(vk - VK_NUMPAD0);

    // MAPVK_VK_TO_CHAR sets the high bit for dead keys.
    const UINT ch = MapVirtualKeyW(vk, MAPVK_VK_TO_CHAR) & 0x7FFFFFFF;
    if (ch != 0) return std::wstring(1, static_cast<wchar_t>(ch));
    return L"Key " + std::to_wstring(vk);
}
