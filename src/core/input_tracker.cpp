#include "input_tracker.h"

namespace inputlens {
namespace {

uint8_t ModifierOf(Key key) {
    switch (key) {
    case Key::Ctrl: return kModCtrl;
    case Key::Alt: return kModAlt;
    case Key::Shift: return kModShift;
    case Key::Meta: return kModMeta;
    default: return 0;
    }
}

bool ProducesText(Key key) { return key >= Key::Space; }

}  // namespace

std::optional<Display> InputTracker::OnKey(const KeyEvent& event, uint64_t nowMs) {
    if (const uint8_t bit = ModifierOf(event.key)) {
        if (event.down) {
            if (modifiers_ & bit) return std::nullopt;  // Auto-repeat of a held modifier.
            tapPending_ = modifiers_ == 0;
            tapKey_ = event.key;
            tapStartMs_ = nowMs;
            modifiers_ |= bit;
            return std::nullopt;
        }
        modifiers_ &= static_cast<uint8_t>(~bit);
        // A modifier alone shows on release, so capitals typed with Shift stay silent.
        const bool tapped = tapPending_ && tapKey_ == event.key && nowMs - tapStartMs_ <= kModifierTapMs;
        tapPending_ = false;
        if (tapped) return Present(event.key, 0, MouseGlyph::None, 0, 1, nowMs);
        return std::nullopt;
    }

    if (!event.down) return std::nullopt;
    tapPending_ = false;

    const bool inCombination = (modifiers_ & (kModCtrl | kModAlt | kModMeta)) != 0;
    // Ordinary typing is dropped here and never stored.
    if (ProducesText(event.key) && !inCombination) return std::nullopt;
    return Present(event.key, event.code, MouseGlyph::None, modifiers_, 1, nowMs);
}

std::optional<Display> InputTracker::OnMouseButton(MouseGlyph button, uint64_t nowMs) {
    tapPending_ = false;
    return Present(Key::Other, 0, button, modifiers_, 1, nowMs);
}

std::optional<Display> InputTracker::OnWheel(int notches, bool horizontal, uint64_t nowMs) {
    tapPending_ = false;
    if (notches == 0) return std::nullopt;
    const MouseGlyph glyph = horizontal ? (notches > 0 ? MouseGlyph::WheelRight : MouseGlyph::WheelLeft)
                                        : (notches > 0 ? MouseGlyph::WheelUp : MouseGlyph::WheelDown);
    return Present(Key::Other, 0, glyph, modifiers_, notches > 0 ? notches : -notches, nowMs);
}

void InputTracker::ResetModifiers() {
    modifiers_ = 0;
    tapPending_ = false;
}

Display InputTracker::Present(Key key, uint32_t code, MouseGlyph glyph, uint8_t modifiers, int amount, uint64_t nowMs) {
    const bool same = hasLast_ && last_.modifiers == modifiers && last_.glyph == glyph &&
                      (glyph != MouseGlyph::None || (last_.key == key && last_.code == code)) &&
                      nowMs - lastMs_ <= kMergeWindowMs;
    last_ = Display{modifiers, key, code, glyph, same ? last_.count + amount : amount};
    hasLast_ = true;
    lastMs_ = nowMs;
    return last_;
}

}  // namespace inputlens
