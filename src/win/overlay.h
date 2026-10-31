#pragma once
#include <string>
#include <windows.h>

#include "input_tracker.h"

using inputlens::MouseGlyph;

// A click-through, never-focused window that shows one short label and fades out.
// It runs no timers while hidden.
class Overlay {
public:
    bool Create(HINSTANCE instance);
    void Destroy();
    // Draws prefix, then the mouse image if any, then suffix.
    void Show(std::wstring prefix, MouseGlyph glyph = MouseGlyph::None, std::wstring suffix = {});
    void Hide();

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    void OnTimer(UINT_PTR id);
    void Prewarm();
    void Render();

    HWND hwnd_ = nullptr;
    ULONG_PTR gdiplusToken_ = 0;
    std::wstring prefix_;
    std::wstring suffix_;
    MouseGlyph glyph_ = MouseGlyph::None;
    int alpha_ = 255;
    bool renderPending_ = false;
};
