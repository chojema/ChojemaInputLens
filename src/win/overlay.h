#pragma once
#include <string>
#include <windows.h>

enum class MouseGlyph { None, Left, Right, Middle, Back, Forward, WheelUp, WheelDown, WheelLeft, WheelRight };

// A click-through, never-focused window that shows one short label and fades out.
// It runs no timers while hidden.
class Overlay {
public:
    bool Create(HINSTANCE instance);
    void Destroy();
    // Draws prefix, then the mouse image if any, then suffix.
    void Show(std::wstring prefix, MouseGlyph glyph = MouseGlyph::None, std::wstring suffix = {});
    void Hide();
    unsigned long long RenderCount() const { return renderCount_; }
    double FirstRenderMs() const { return firstRenderMs_; }
    double MaxRenderMs() const { return maxRenderMs_; }

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
    unsigned long long renderCount_ = 0;
    double firstRenderMs_ = 0;
    double maxRenderMs_ = 0;
};
