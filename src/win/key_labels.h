#pragma once
#include <optional>
#include <string>
#include <windows.h>

#include "input_tracker.h"

void SetKeyLabelsEnglish(bool english);

// Translates a Raw Input key. Returns nothing for the synthetic codes Windows emits around other keys.
std::optional<inputlens::KeyEvent> TranslateKey(UINT vk, bool extended, bool up);

// Bits of the tracked modifiers that are still physically held.
uint8_t HeldModifiers(uint8_t tracked);

// Text before the mouse image, or the full key label when there is no image.
std::wstring DisplayPrefix(const inputlens::Display& display);
