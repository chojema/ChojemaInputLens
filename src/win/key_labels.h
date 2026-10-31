#pragma once
#include <string>
#include <windows.h>

// Label for a key shown on its own: function, navigation and IME keys.
// Returns nullptr for keys that produce text, which are not shown by themselves.
const wchar_t* StandaloneKeyLabel(UINT vk, bool extended);

// Label for any non-modifier key used inside a combination such as Ctrl + C.
std::wstring ComboKeyLabel(UINT vk, bool extended);
