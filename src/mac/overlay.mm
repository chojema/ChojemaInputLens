#import "overlay.h"
#include <algorithm>

using inputlens::MouseGlyph;

@interface OverlayPanel : NSPanel
@end
@implementation OverlayPanel
- (BOOL)canBecomeKeyWindow { return NO; }
- (BOOL)canBecomeMainWindow { return NO; }
@end

@interface OverlayView : NSView
@property(copy) NSString* prefix;
@property(copy) NSString* suffix;
@property MouseGlyph glyph;
- (NSSize)preferredSize;
@end

@implementation OverlayView
- (BOOL)isFlipped { return YES; }
- (NSDictionary*)textAttributes {
    return @{NSFontAttributeName: [NSFont systemFontOfSize:26 weight:NSFontWeightSemibold],
             NSForegroundColorAttributeName: [NSColor colorWithWhite:0.97 alpha:1]};
}
- (NSSize)preferredSize {
    NSDictionary* attributes = self.textAttributes;
    return NSMakeSize(46 + [self.prefix sizeWithAttributes:attributes].width +
                      [self.suffix sizeWithAttributes:attributes].width +
                      (self.glyph == MouseGlyph::None ? 0 : 46), 66);
}
- (void)drawMouseAt:(NSPoint)origin {
    [NSGraphicsContext saveGraphicsState];
    NSAffineTransform* transform = [NSAffineTransform transform];
    [transform translateXBy:origin.x yBy:origin.y];
    [transform concat];
    NSColor* ink = [NSColor colorWithWhite:0.96 alpha:1];
    NSColor* accent = [NSColor colorWithRed:0.37 green:0.78 blue:1 alpha:1];
    NSBezierPath* body = [NSBezierPath bezierPathWithRoundedRect:NSMakeRect(9, 2, 27, 36) xRadius:12 yRadius:12];
    if (self.glyph == MouseGlyph::Left || self.glyph == MouseGlyph::Right) {
        [NSGraphicsContext saveGraphicsState];
        [body addClip];
        [accent setFill];
        NSRectFill(NSMakeRect(self.glyph == MouseGlyph::Left ? 9 : 22.5, 2, 13.5, 16));
        [NSGraphicsContext restoreGraphicsState];
    }
    [ink setStroke];
    body.lineWidth = 2;
    [body stroke];
    NSBezierPath* split = [NSBezierPath bezierPath];
    [split moveToPoint:NSMakePoint(9, 18)];
    [split lineToPoint:NSMakePoint(36, 18)];
    [split moveToPoint:NSMakePoint(22.5, 2)];
    [split lineToPoint:NSMakePoint(22.5, 18)];
    split.lineWidth = 1.4;
    [split stroke];
    const bool wheel = self.glyph == MouseGlyph::Middle || self.glyph >= MouseGlyph::WheelUp;
    [(wheel ? accent : ink) setFill];
    [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(20, 6, 5, 10) xRadius:2.5 yRadius:2.5] fill];
    if (self.glyph == MouseGlyph::Back || self.glyph == MouseGlyph::Forward) {
        [accent setFill];
        [[NSBezierPath bezierPathWithRoundedRect:NSMakeRect(3, self.glyph == MouseGlyph::Forward ? 12 : 23, 4, 8)
                                         xRadius:2 yRadius:2] fill];
    }
    CGFloat dx = 0, dy = 0, x = 41, y = 20;
    switch (self.glyph) {
    case MouseGlyph::WheelUp: dy = -1; break;
    case MouseGlyph::WheelDown: dy = 1; break;
    case MouseGlyph::WheelLeft: dx = -1; x = 3; break;
    case MouseGlyph::WheelRight: dx = 1; break;
    default: break;
    }
    if (dx || dy) {
        NSBezierPath* arrow = [NSBezierPath bezierPath];
        [arrow moveToPoint:NSMakePoint(x + dx * 4, y + dy * 4)];
        [arrow lineToPoint:NSMakePoint(x - dx * 4 - dy * 3, y - dy * 4 - dx * 3)];
        [arrow lineToPoint:NSMakePoint(x - dx * 4 + dy * 3, y - dy * 4 + dx * 3)];
        [arrow closePath];
        [accent setFill];
        [arrow fill];
    }
    [NSGraphicsContext restoreGraphicsState];
}
- (void)drawRect:(NSRect)dirtyRect {
    (void)dirtyRect;
    NSBezierPath* background = [NSBezierPath bezierPathWithRoundedRect:NSInsetRect(self.bounds, 0.5, 0.5)
                                                            xRadius:13 yRadius:13];
    [[NSColor colorWithRed:0.06 green:0.075 blue:0.11 alpha:0.9] setFill];
    [background fill];
    [[NSColor colorWithWhite:1 alpha:0.2] setStroke];
    [background stroke];
    NSDictionary* attributes = self.textAttributes;
    CGFloat textY = (NSHeight(self.bounds) - [@"가A" sizeWithAttributes:attributes].height) / 2;
    CGFloat x = 23;
    [self.prefix drawAtPoint:NSMakePoint(x, textY) withAttributes:attributes];
    x += [self.prefix sizeWithAttributes:attributes].width;
    if (self.glyph != MouseGlyph::None) {
        [self drawMouseAt:NSMakePoint(x, 13)];
        x += 46;
    }
    [self.suffix drawAtPoint:NSMakePoint(x, textY) withAttributes:attributes];
}
@end

@interface InputOverlay ()
@property OverlayPanel* panel;
@property OverlayView* view;
@property NSTimer* renderTimer;
@property NSTimer* hideTimer;
@property NSTimer* fadeTimer;
@end

@implementation InputOverlay
- (instancetype)init {
    self = [super init];
    if (!self) return nil;
    _panel = [[OverlayPanel alloc] initWithContentRect:NSMakeRect(0, 0, 180, 66)
        styleMask:NSWindowStyleMaskBorderless | NSWindowStyleMaskNonactivatingPanel
        backing:NSBackingStoreBuffered defer:NO];
    _panel.opaque = NO;
    _panel.backgroundColor = NSColor.clearColor;
    _panel.hasShadow = NO;
    _panel.ignoresMouseEvents = YES;
    _panel.hidesOnDeactivate = NO;
    _panel.level = NSScreenSaverWindowLevel;
    _panel.collectionBehavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
        NSWindowCollectionBehaviorFullScreenAuxiliary | NSWindowCollectionBehaviorStationary |
        NSWindowCollectionBehaviorIgnoresCycle;
    _view = [[OverlayView alloc] initWithFrame:_panel.contentView.bounds];
    _view.prefix = @"";
    _view.suffix = @"";
    _panel.contentView = _view;
    return self;
}
- (void)showPrefix:(NSString*)prefix glyph:(MouseGlyph)glyph count:(int)count {
    self.view.prefix = prefix;
    self.view.glyph = glyph;
    self.view.suffix = count > 1 ? [NSString stringWithFormat:@" ×%d", count] : @"";
    if (self.renderTimer) return;
    __weak InputOverlay* weakSelf = self;
    self.renderTimer = [NSTimer timerWithTimeInterval:1.0 / 60 repeats:NO block:^(NSTimer*) {
        InputOverlay* overlay = weakSelf;
        overlay.renderTimer = nil;
        [overlay render];
    }];
    [NSRunLoop.mainRunLoop addTimer:self.renderTimer forMode:NSRunLoopCommonModes];
}
- (void)render {
    [self.hideTimer invalidate];
    [self.fadeTimer invalidate];
    NSPoint mouse = NSEvent.mouseLocation;
    NSScreen* screen = NSScreen.mainScreen;
    for (NSScreen* candidate in NSScreen.screens) {
        if (NSPointInRect(mouse, candidate.frame)) { screen = candidate; break; }
    }
    if (!screen) return;
    NSRect work = screen.visibleFrame;
    NSSize size = self.view.preferredSize;
    size.width = std::min(size.width, work.size.width);
    [self.panel setFrame:NSMakeRect(NSMidX(work) - size.width / 2, NSMinY(work) + 56, size.width, size.height)
                 display:NO];
    self.view.needsDisplay = YES;
    self.panel.alphaValue = 1;
    [self.panel orderFrontRegardless];
    __weak InputOverlay* weakSelf = self;
    self.hideTimer = [NSTimer timerWithTimeInterval:0.9 repeats:NO block:^(NSTimer*) {
        InputOverlay* overlay = weakSelf;
        overlay.fadeTimer = [NSTimer timerWithTimeInterval:0.04 repeats:YES block:^(NSTimer*) {
            InputOverlay* current = weakSelf;
            current.panel.alphaValue = std::max(0.0, current.panel.alphaValue - 0.16);
            if (current.panel.alphaValue <= 0) [current hide];
        }];
        [NSRunLoop.mainRunLoop addTimer:overlay.fadeTimer forMode:NSRunLoopCommonModes];
    }];
    [NSRunLoop.mainRunLoop addTimer:self.hideTimer forMode:NSRunLoopCommonModes];
}
- (void)hide {
    [self.renderTimer invalidate];
    [self.hideTimer invalidate];
    [self.fadeTimer invalidate];
    self.renderTimer = nil;
    self.hideTimer = nil;
    self.fadeTimer = nil;
    [self.panel orderOut:nil];
}
@end
