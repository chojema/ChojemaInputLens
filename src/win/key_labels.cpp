#include "key_labels.h"

using inputlens::Display;
using inputlens::Key;
using inputlens::KeyEvent;
using inputlens::MouseGlyph;

namespace {

bool g_english = false;

Key MapKey(UINT vk, bool extended, uint32_t& code) {
    if (vk >= VK_F1 && vk <= VK_F24) {
        code = vk - VK_F1 + 1;
        return Key::Function;
    }
    if (vk >= VK_NUMPAD0 && vk <= VK_NUMPAD9) {
        code = vk - VK_NUMPAD0;
        return Key::Numpad;
    }
    // Without the extended flag these come from the numeric keypad, which can type digits.
    if (extended) {
        switch (vk) {
        case VK_INSERT: return Key::Insert;
        case VK_DELETE: return Key::Delete;
        case VK_HOME: return Key::Home;
        case VK_END: return Key::End;
        case VK_PRIOR: return Key::PageUp;
        case VK_NEXT: return Key::PageDown;
        case VK_LEFT: return Key::Left;
        case VK_UP: return Key::Up;
        case VK_RIGHT: return Key::Right;
        case VK_DOWN: return Key::Down;
        default: break;
        }
    } else {
        if (vk == VK_INSERT) {
            code = 0;
            return Key::Numpad;
        }
        if (vk == VK_DELETE) return Key::NumpadDecimal;
    }
    switch (vk) {
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL: return Key::Ctrl;
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU: return Key::Alt;
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT: return Key::Shift;
    case VK_LWIN:
    case VK_RWIN: return Key::Meta;
    case VK_ESCAPE: return Key::Esc;
    case VK_SNAPSHOT: return Key::PrintScreen;
    case VK_PAUSE: return Key::Pause;
    case VK_APPS: return Key::Menu;
    case VK_CAPITAL: return Key::CapsLock;
    case VK_NUMLOCK: return Key::NumLock;
    case VK_SCROLL: return Key::ScrollLock;
    case VK_HANGUL: return Key::ImeToggle;
    case VK_HANJA: return Key::ImeConvert;
    case VK_SPACE: return Key::Space;
    case VK_RETURN: return Key::Enter;
    case VK_TAB: return Key::Tab;
    case VK_BACK: return Key::Backspace;
    default:
        code = vk;
        return Key::Other;
    }
}

std::wstring KeyLabel(Key key, uint32_t code) {
    switch (key) {
    case Key::Ctrl: return L"Ctrl";
    case Key::Alt: return L"Alt";
    case Key::Shift: return L"Shift";
    case Key::Meta: return L"Win";
    case Key::Function: return L"F" + std::to_wstring(code);
    case Key::Esc: return L"Esc";
    case Key::Insert: return L"Insert";
    case Key::Delete: return L"Delete";
    case Key::Home: return L"Home";
    case Key::End: return L"End";
    case Key::PageUp: return L"Page Up";
    case Key::PageDown: return L"Page Down";
    case Key::Left: return L"←";
    case Key::Up: return L"↑";
    case Key::Right: return L"→";
    case Key::Down: return L"↓";
    case Key::PrintScreen: return L"Print Screen";
    case Key::Pause: return L"Pause";
    case Key::Menu: return L"Menu";
    case Key::CapsLock: return L"Caps Lock";
    case Key::NumLock: return L"Num Lock";
    case Key::ScrollLock: return L"Scroll Lock";
    case Key::ImeToggle: return g_english ? L"Han/Eng" : L"한/영";
    case Key::ImeConvert: return g_english ? L"Hanja" : L"한자";
    case Key::Space: return L"Space";
    case Key::Enter: return L"Enter";
    case Key::Tab: return L"Tab";
    case Key::Backspace: return L"Backspace";
    case Key::Numpad: return L"Num " + std::to_wstring(code);
    case Key::NumpadDecimal: return L"Num .";
    case Key::Other: break;
    }
    // MAPVK_VK_TO_CHAR sets the high bit for dead keys.
    const UINT ch = MapVirtualKeyW(code, MAPVK_VK_TO_CHAR) & 0x7FFFFFFF;
    if (ch != 0) return std::wstring(1, static_cast<wchar_t>(ch));
    return L"Key " + std::to_wstring(code);
}

}  // namespace

void SetKeyLabelsEnglish(bool english) { g_english = english; }

std::optional<KeyEvent> TranslateKey(UINT vk, bool extended, bool up) {
    if (vk == 0 || vk == 0xFF || (vk == VK_SHIFT && extended)) return std::nullopt;
    uint32_t code = 0;
    const Key key = MapKey(vk, extended, code);
    return KeyEvent{key, code, !up};
}

uint8_t HeldModifiers(uint8_t tracked) {
    const auto held = [](int vk) { return (GetAsyncKeyState(vk) & 0x8000) != 0; };
    uint8_t result = 0;
    if ((tracked & inputlens::kModCtrl) && held(VK_CONTROL)) result |= inputlens::kModCtrl;
    if ((tracked & inputlens::kModAlt) && held(VK_MENU)) result |= inputlens::kModAlt;
    if ((tracked & inputlens::kModShift) && held(VK_SHIFT)) result |= inputlens::kModShift;
    if ((tracked & inputlens::kModMeta) && (held(VK_LWIN) || held(VK_RWIN))) result |= inputlens::kModMeta;
    return result;
}

std::wstring DisplayPrefix(const Display& display) {
    std::wstring text;
    if (display.modifiers & inputlens::kModCtrl) text += L"Ctrl + ";
    if (display.modifiers & inputlens::kModAlt) text += L"Alt + ";
    if (display.modifiers & inputlens::kModShift) text += L"Shift + ";
    if (display.modifiers & inputlens::kModMeta) text += L"Win + ";
    if (display.glyph == MouseGlyph::None) text += KeyLabel(display.key, display.code);
    return text;
}
