#import "key_labels.h"
#import <Carbon/Carbon.h>

using namespace inputlens;

KeyEvent TranslateKey(CGKeyCode code, bool down) {
    Key key = Key::Other;
    uint32_t value = code;
    // Function-key scan codes are not contiguous on macOS.
    constexpr CGKeyCode functions[] = {kVK_F1, kVK_F2, kVK_F3, kVK_F4, kVK_F5,
        kVK_F6, kVK_F7, kVK_F8, kVK_F9, kVK_F10, kVK_F11, kVK_F12,
        kVK_F13, kVK_F14, kVK_F15, kVK_F16, kVK_F17, kVK_F18, kVK_F19, kVK_F20};
    for (uint32_t i = 0; i < 20; ++i) {
        if (code == functions[i]) return {Key::Function, i + 1, down};
    }
    switch (code) {
    case kVK_Control: case kVK_RightControl: key = Key::Ctrl; break;
    case kVK_Option: case kVK_RightOption: key = Key::Alt; break;
    case kVK_Shift: case kVK_RightShift: key = Key::Shift; break;
    case kVK_Command: case kVK_RightCommand: key = Key::Meta; break;
    case kVK_Escape: key = Key::Esc; break;
    case kVK_ForwardDelete: key = Key::Delete; break;
    case kVK_Help: key = Key::Insert; break;
    case kVK_Home: key = Key::Home; break;
    case kVK_End: key = Key::End; break;
    case kVK_PageUp: key = Key::PageUp; break;
    case kVK_PageDown: key = Key::PageDown; break;
    case kVK_LeftArrow: key = Key::Left; break;
    case kVK_RightArrow: key = Key::Right; break;
    case kVK_UpArrow: key = Key::Up; break;
    case kVK_DownArrow: key = Key::Down; break;
    case kVK_CapsLock: key = Key::CapsLock; break;
    case kVK_JIS_Eisu: case kVK_JIS_Kana: key = Key::ImeToggle; break;
    case kVK_Space: key = Key::Space; break;
    case kVK_Return: case kVK_ANSI_KeypadEnter: key = Key::Enter; break;
    case kVK_Tab: key = Key::Tab; break;
    case kVK_Delete: key = Key::Backspace; break;
    default: break;
    }
    return {key, value, down};
}

uint8_t ModifierBits(CGEventFlags flags) {
    return ((flags & kCGEventFlagMaskControl) ? kModCtrl : 0) |
           ((flags & kCGEventFlagMaskAlternate) ? kModAlt : 0) |
           ((flags & kCGEventFlagMaskShift) ? kModShift : 0) |
           ((flags & kCGEventFlagMaskCommand) ? kModMeta : 0);
}

static NSString* KeyLabel(Key key, uint32_t code, bool english) {
    switch (key) {
    case Key::Ctrl: return @"Ctrl";
    case Key::Alt: return @"Option";
    case Key::Shift: return @"Shift";
    case Key::Meta: return @"Cmd";
    case Key::Function: return [NSString stringWithFormat:@"F%u", code];
    case Key::Esc: return @"Esc";
    case Key::Insert: return @"Help";
    case Key::Delete: return @"⌦";
    case Key::Home: return @"Home";
    case Key::End: return @"End";
    case Key::PageUp: return @"Page Up";
    case Key::PageDown: return @"Page Down";
    case Key::Left: return @"←";
    case Key::Up: return @"↑";
    case Key::Right: return @"→";
    case Key::Down: return @"↓";
    case Key::CapsLock: return @"Caps Lock";
    case Key::ImeToggle: return english ? @"Input source" : @"입력 소스";
    case Key::Space: return @"Space";
    case Key::Enter: return @"Return";
    case Key::Tab: return @"Tab";
    case Key::Backspace: return @"Delete";
    default: break;
    }
    // Resolve only visible shortcuts; ordinary typed text is never read.
    TISInputSourceRef source = TISCopyCurrentASCIICapableKeyboardLayoutInputSource();
    NSString* label = nil;
    if (source) {
        CFDataRef data = (CFDataRef)TISGetInputSourceProperty(source, kTISPropertyUnicodeKeyLayoutData);
        if (data) {
            const auto* layout = reinterpret_cast<const UCKeyboardLayout*>(CFDataGetBytePtr(data));
            UInt32 deadKey = 0;
            UniChar chars[8];
            UniCharCount length = 0;
            if (UCKeyTranslate(layout, static_cast<UInt16>(code), kUCKeyActionDisplay, 0,
                               LMGetKbdType(), kUCKeyTranslateNoDeadKeysBit, &deadKey,
                               8, &length, chars) == noErr && length && chars[0] >= 0x20) {
                label = [[NSString stringWithCharacters:chars length:length] uppercaseString];
            }
        }
        CFRelease(source);
    }
    return label ? label : [NSString stringWithFormat:@"Key %u", code];
}

NSString* DisplayPrefix(const Display& display, bool english) {
    NSMutableString* text = [NSMutableString string];
    if (display.modifiers & kModCtrl) [text appendString:@"Ctrl + "];
    if (display.modifiers & kModAlt) [text appendString:@"Option + "];
    if (display.modifiers & kModShift) [text appendString:@"Shift + "];
    if (display.modifiers & kModMeta) [text appendString:@"Cmd + "];
    if (display.glyph == MouseGlyph::None) [text appendString:KeyLabel(display.key, display.code, english)];
    return text;
}
