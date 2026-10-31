#include "overlay.h"

#include <algorithm>
using std::max;
using std::min;
#include <objidl.h>
#include <gdiplus.h>
#include <shellscalingapi.h>

namespace {
constexpr wchar_t kClassName[] = L"ChojemaInputLensOverlay";
constexpr UINT_PTR kTimerRender = 1;
constexpr UINT_PTR kTimerHold = 2;
constexpr UINT_PTR kTimerFade = 3;
constexpr UINT kRenderDelayMs = 16;
constexpr UINT kHoldMs = 1200;
constexpr UINT kFadeStepMs = 16;
constexpr int kFadeStep = 28;

void AddRoundedRect(Gdiplus::GraphicsPath& path, Gdiplus::RectF r, float radius) {
    const float d = radius * 2;
    path.AddArc(r.X, r.Y, d, d, 180, 90);
    path.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    path.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    path.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    path.CloseFigure();
}

// Design box: kGlyphWidth x kGlyphHeight. The body is centered, leaving side margins for
// the thumb button and direction arrows.
constexpr float kGlyphWidth = 44.0f;
constexpr float kGlyphHeight = 36.0f;

void DrawMouseGlyph(Gdiplus::Graphics& g, MouseGlyph glyph, float x, float y, float s) {
    using namespace Gdiplus;
    const Color ink(255, 246, 249, 252);
    SolidBrush inkBrush(ink);
    SolidBrush accent(Color(255, 110, 243, 197));

    const float bodyX = x + 10 * s, bodyY = y + 1 * s, bodyW = 24 * s, bodyH = 34 * s;
    const float split = bodyY + 14 * s, centerX = bodyX + bodyW / 2;
    GraphicsPath body;
    AddRoundedRect(body, RectF(bodyX, bodyY, bodyW, bodyH), 11 * s);

    if (glyph == MouseGlyph::Left || glyph == MouseGlyph::Right) {
        const GraphicsState state = g.Save();
        g.SetClip(&body);
        g.FillRectangle(&accent, glyph == MouseGlyph::Left ? bodyX : centerX, bodyY, bodyW / 2, split - bodyY);
        g.Restore(state);
    }

    Pen pen(ink, 2 * s);
    g.DrawPath(&pen, &body);
    g.DrawLine(&pen, bodyX, split, bodyX + bodyW, split);
    g.DrawLine(&pen, centerX, bodyY, centerX, split);

    const bool wheelActive = glyph == MouseGlyph::Middle || glyph == MouseGlyph::WheelUp ||
                             glyph == MouseGlyph::WheelDown || glyph == MouseGlyph::WheelLeft ||
                             glyph == MouseGlyph::WheelRight;
    GraphicsPath wheel;
    AddRoundedRect(wheel, RectF(centerX - 3 * s, bodyY + 4 * s, 6 * s, 11 * s), 2.9f * s);
    g.FillPath(wheelActive ? &accent : &inkBrush, &wheel);

    if (glyph == MouseGlyph::Back || glyph == MouseGlyph::Forward) {
        GraphicsPath side;
        const float sideY = bodyY + (glyph == MouseGlyph::Forward ? 12 : 21) * s;
        AddRoundedRect(side, RectF(bodyX - 5 * s, sideY, 4 * s, 8 * s), 1.9f * s);
        g.FillPath(&accent, &side);
    }

    float dx = 0, dy = 0;
    float arrowX = x + 39.5f * s;
    switch (glyph) {
    case MouseGlyph::WheelUp: dy = -1; break;
    case MouseGlyph::WheelDown: dy = 1; break;
    case MouseGlyph::WheelLeft: dx = -1; arrowX = x + 4.5f * s; break;
    case MouseGlyph::WheelRight:
    case MouseGlyph::Forward: dx = 1; break;
    case MouseGlyph::Back: dx = -1; break;
    default: break;
    }
    if (dx != 0 || dy != 0) {
        const float r = 4.5f * s, arrowY = y + kGlyphHeight / 2 * s;
        const PointF triangle[3] = {PointF(arrowX + dx * r, arrowY + dy * r),
                                    PointF(arrowX - dx * r - dy * r, arrowY - dy * r - dx * r),
                                    PointF(arrowX - dx * r + dy * r, arrowY - dy * r + dx * r)};
        g.FillPolygon(&accent, triangle, 3);
    }
}
}  // namespace

bool Overlay::Create(HINSTANCE instance) {
    Gdiplus::GdiplusStartupInput startup;
    if (Gdiplus::GdiplusStartup(&gdiplusToken_, &startup, nullptr) != Gdiplus::Ok) return false;

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc)) return false;

    hwnd_ = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        kClassName, L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, instance, this);
    if (!hwnd_) return false;
    Prewarm();
    return true;
}

// GDI+ loads its font data on first use, which otherwise delays the first label by hundreds of ms.
void Overlay::Prewarm() {
    using namespace Gdiplus;
    FontFamily malgun(L"Malgun Gothic");
    const FontFamily* family = malgun.IsAvailable() ? &malgun : FontFamily::GenericSansSerif();
    Font font(family, 26.0f, FontStyleBold, UnitPixel);
    Bitmap canvas(8, 8, PixelFormat32bppPARGB);
    Graphics g(&canvas);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
    SolidBrush ink(Color(255, 255, 255, 255));
    g.DrawString(L"가A", -1, &font, PointF(0, 0), &ink);
}

void Overlay::Destroy() {
    if (hwnd_) DestroyWindow(hwnd_);
    hwnd_ = nullptr;
    if (gdiplusToken_) Gdiplus::GdiplusShutdown(gdiplusToken_);
    gdiplusToken_ = 0;
}

void Overlay::Show(std::wstring prefix, MouseGlyph glyph, std::wstring suffix) {
    prefix_ = std::move(prefix);
    glyph_ = glyph;
    suffix_ = std::move(suffix);
    // Fast bursts such as wheel spins collapse into one render per timer tick.
    if (!renderPending_) {
        renderPending_ = true;
        SetTimer(hwnd_, kTimerRender, kRenderDelayMs, nullptr);
    }
}

void Overlay::Hide() {
    KillTimer(hwnd_, kTimerRender);
    KillTimer(hwnd_, kTimerHold);
    KillTimer(hwnd_, kTimerFade);
    renderPending_ = false;
    ShowWindow(hwnd_, SW_HIDE);
}

void Overlay::OnTimer(UINT_PTR id) {
    KillTimer(hwnd_, id);
    switch (id) {
    case kTimerRender:
        renderPending_ = false;
        KillTimer(hwnd_, kTimerFade);
        alpha_ = 255;
        {
            LARGE_INTEGER begin, end, frequency;
            QueryPerformanceCounter(&begin);
            Render();
            QueryPerformanceCounter(&end);
            QueryPerformanceFrequency(&frequency);
            const double ms = (end.QuadPart - begin.QuadPart) * 1000.0 / frequency.QuadPart;
            if (renderCount_ == 1) firstRenderMs_ = ms;
            maxRenderMs_ = max(maxRenderMs_, ms);
        }
        SetTimer(hwnd_, kTimerHold, kHoldMs, nullptr);
        break;
    case kTimerHold:
        SetTimer(hwnd_, kTimerFade, kFadeStepMs, nullptr);
        break;
    case kTimerFade: {
        alpha_ -= kFadeStep;
        if (alpha_ <= 0) {
            Hide();
            break;
        }
        BLENDFUNCTION blend{AC_SRC_OVER, 0, static_cast<BYTE>(alpha_), AC_SRC_ALPHA};
        UpdateLayeredWindow(hwnd_, nullptr, nullptr, nullptr, nullptr, nullptr, 0, &blend, ULW_ALPHA);
        SetTimer(hwnd_, kTimerFade, kFadeStepMs, nullptr);
        break;
    }
    default:
        break;
    }
}

void Overlay::Render() {
    using namespace Gdiplus;
    ++renderCount_;

    POINT cursor{};
    GetCursorPos(&cursor);
    HMONITOR monitor = MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(monitor, &mi);
    UINT dpiX = 96, dpiY = 96;
    GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
    const float scale = dpiX / 96.0f;

    FontFamily malgun(L"Malgun Gothic");
    const FontFamily* family = malgun.IsAvailable() ? &malgun : FontFamily::GenericSansSerif();
    Font font(family, 26.0f * scale, FontStyleBold, UnitPixel);

    StringFormat format(StringFormat::GenericTypographic());
    format.SetFormatFlags(format.GetFormatFlags() | StringFormatFlagsNoWrap |
                          StringFormatFlagsMeasureTrailingSpaces);

    float prefixWidth = 0, suffixWidth = 0, lineHeight = 0;
    {
        Bitmap probe(1, 1, PixelFormat32bppPARGB);
        Graphics g(&probe);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        RectF bounds;
        g.MeasureString(L"가A", -1, &font, PointF(0, 0), &format, &bounds);
        lineHeight = bounds.Height;
        if (!prefix_.empty()) {
            g.MeasureString(prefix_.c_str(), -1, &font, PointF(0, 0), &format, &bounds);
            prefixWidth = bounds.Width;
        }
        if (!suffix_.empty()) {
            g.MeasureString(suffix_.c_str(), -1, &font, PointF(0, 0), &format, &bounds);
            suffixWidth = bounds.Width;
        }
    }
    const bool hasGlyph = glyph_ != MouseGlyph::None;
    const float glyphWidth = hasGlyph ? kGlyphWidth * scale : 0;
    const float contentHeight = max(lineHeight, hasGlyph ? kGlyphHeight * scale : 0.0f);

    const int paddingX = static_cast<int>(22 * scale);
    const int paddingY = static_cast<int>(12 * scale);
    const int workWidth = mi.rcWork.right - mi.rcWork.left;
    const int width = min(static_cast<int>(prefixWidth + glyphWidth + suffixWidth) + paddingX * 2 + 2, workWidth);
    const int height = static_cast<int>(contentHeight) + paddingY * 2 + 2;
    const POINT position{mi.rcWork.left + (workWidth - width) / 2,
                         mi.rcWork.bottom - height - static_cast<int>(56 * scale)};

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC memory = CreateCompatibleDC(nullptr);
    HBITMAP dib = CreateDIBSection(memory, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!memory || !dib) {
        if (dib) DeleteObject(dib);
        if (memory) DeleteDC(memory);
        return;
    }
    HGDIOBJ previous = SelectObject(memory, dib);
    {
        Bitmap canvas(width, height, width * 4, PixelFormat32bppPARGB, static_cast<BYTE*>(bits));
        Graphics g(&canvas);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

        const RectF panel(0.5f, 0.5f, width - 1.0f, height - 1.0f);
        GraphicsPath path;
        AddRoundedRect(path, panel, 13.0f * scale);
        SolidBrush fill(Color(226, 15, 19, 28));
        g.FillPath(&fill, &path);
        Pen border(Color(52, 255, 255, 255), max(1.0f, scale));
        g.DrawPath(&border, &path);

        SolidBrush ink(Color(255, 246, 249, 252));
        const float textY = paddingY + (contentHeight - lineHeight) / 2;
        float x = static_cast<float>(paddingX);
        if (!prefix_.empty()) g.DrawString(prefix_.c_str(), -1, &font, PointF(x, textY), &format, &ink);
        x += prefixWidth;
        if (hasGlyph) {
            DrawMouseGlyph(g, glyph_, x, paddingY + (contentHeight - kGlyphHeight * scale) / 2, scale);
        }
        x += glyphWidth;
        if (!suffix_.empty()) g.DrawString(suffix_.c_str(), -1, &font, PointF(x, textY), &format, &ink);
    }

    POINT origin{0, 0};
    SIZE size{width, height};
    BLENDFUNCTION blend{AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hwnd_, nullptr, const_cast<POINT*>(&position), &size, memory, &origin, 0, &blend,
                        ULW_ALPHA);
    SelectObject(memory, previous);
    DeleteObject(dib);
    DeleteDC(memory);

    SetWindowPos(hwnd_, HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
}

LRESULT CALLBACK Overlay::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lp);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }
    auto* self = reinterpret_cast<Overlay*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_TIMER:
        if (self) self->OnTimer(wp);
        return 0;
    case WM_NCHITTEST:
        return HTTRANSPARENT;
    case WM_MOUSEACTIVATE:
        return MA_NOACTIVATE;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}
