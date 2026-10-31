#pragma once
#include <cstdint>
#include <optional>

// Decides what to display for each input event. This code knows no operating system API:
// platform layers translate native events into KeyEvent and turn Display into text.
namespace inputlens {

enum ModifierBit : uint8_t {
    kModCtrl = 1 << 0,
    kModAlt = 1 << 1,
    kModShift = 1 << 2,
    kModMeta = 1 << 3,  // Windows key or Command key.
};

enum class Key : uint8_t {
    // Modifiers.
    Ctrl, Alt, Shift, Meta,
    // Shown on their own. Function uses code 1-24.
    Function, Esc, Insert, Delete, Home, End, PageUp, PageDown, Left, Up, Right, Down,
    PrintScreen, Pause, Menu, CapsLock, NumLock, ScrollLock, ImeToggle, ImeConvert,
    // Produce text, so they appear only inside a combination.
    // Numpad uses code 0-9. Other carries the platform's key code.
    Space, Enter, Tab, Backspace, Numpad, NumpadDecimal, Other,
};

enum class MouseGlyph : uint8_t { None, Left, Right, Middle, Back, Forward, WheelUp, WheelDown, WheelLeft, WheelRight };

struct KeyEvent {
    Key key;
    uint32_t code;
    bool down;
};

struct Display {
    uint8_t modifiers;
    Key key;             // Meaningful only when glyph is None.
    uint32_t code;       // Meaningful only when glyph is None.
    MouseGlyph glyph;
    int count;
};

class InputTracker {
public:
    static constexpr uint64_t kMergeWindowMs = 700;
    static constexpr uint64_t kModifierTapMs = 1000;

    std::optional<Display> OnKey(const KeyEvent& event, uint64_t nowMs);
    std::optional<Display> OnMouseButton(MouseGlyph button, uint64_t nowMs);
    // Positive notches are up or right. Zero only cancels a pending modifier tap.
    std::optional<Display> OnWheel(int notches, bool horizontal, uint64_t nowMs);

    // Any mouse activity means a held modifier was not a tap.
    void CancelTap() { tapPending_ = false; }

    // Clears modifiers whose key-up was lost, such as when the lock screen takes over.
    // Pass the bits of the tracked modifiers that are still physically held.
    void Reconcile(uint8_t stillHeld) { modifiers_ &= stillHeld; }

    void ResetModifiers();
    uint8_t Modifiers() const { return modifiers_; }

private:
    Display Present(Key key, uint32_t code, MouseGlyph glyph, uint8_t modifiers, int amount, uint64_t nowMs);

    uint8_t modifiers_ = 0;
    bool tapPending_ = false;
    Key tapKey_ = Key::Ctrl;
    uint64_t tapStartMs_ = 0;

    bool hasLast_ = false;
    Display last_{};
    uint64_t lastMs_ = 0;
};

}  // namespace inputlens
