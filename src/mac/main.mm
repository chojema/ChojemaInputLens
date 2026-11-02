#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import "key_labels.h"
#import "overlay.h"

using namespace inputlens;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSMenuDelegate> {
@public
    InputTracker tracker;
    CFMachPortRef eventTap;
    CFRunLoopSourceRef eventSource;
}
@property NSStatusItem* statusItem;
@property InputOverlay* overlay;
@property BOOL english;
@property BOOL paused;
@property BOOL showMouse;
- (void)handleEvent:(CGEventRef)event type:(CGEventType)type;
- (void)startInput;
- (void)stopInput;
@end

static CGEventRef InputCallback(CGEventTapProxy, CGEventType type, CGEventRef event, void* context) {
    AppDelegate* app = (__bridge AppDelegate*)context;
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        app->tracker.ResetModifiers();
        if (app->eventTap && !app.paused) CGEventTapEnable(app->eventTap, true);
    } else if (!app.paused) {
        [app handleEvent:event type:type];
    }
    return event; // Passive listener: never consumes or changes input.
}

@implementation AppDelegate
- (NSString*)text:(NSString*)korean english:(NSString*)english { return self.english ? english : korean; }
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    (void)notification;
    NSString* saved = [NSUserDefaults.standardUserDefaults stringForKey:@"DisplayLanguage"];
    self.english = saved ? [saved isEqualToString:@"en"] :
        [NSLocale.preferredLanguages.firstObject hasPrefix:@"en"];
    self.showMouse = YES;
    self.overlay = [InputOverlay new];
    self.statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength];
    self.statusItem.button.image = [NSImage imageWithSystemSymbolName:@"keyboard" accessibilityDescription:@"Chojema InputLens"];
    self.statusItem.button.toolTip = @"Chojema InputLens";
    [self rebuildMenu];
    [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self selector:@selector(sessionChanged:)
        name:NSWorkspaceSessionDidResignActiveNotification object:nil];
    [NSWorkspace.sharedWorkspace.notificationCenter addObserver:self selector:@selector(sessionChanged:)
        name:NSWorkspaceDidWakeNotification object:nil];
    [self startInput];
    if (!eventTap) [self showPermissionHelp:nil];
}
- (void)sessionChanged:(NSNotification*)notification {
    (void)notification;
    tracker = InputTracker{};
    [self.overlay hide];
}
- (void)applicationDidBecomeActive:(NSNotification*)notification {
    (void)notification;
    if (self.statusItem && !eventTap && !self.paused) [self startInput];
}
- (void)present:(const std::optional<Display>&)display {
    if (display) [self.overlay showPrefix:DisplayPrefix(*display, self.english) glyph:display->glyph count:display->count];
}
- (void)handleEvent:(CGEventRef)event type:(CGEventType)type {
    const uint64_t now = static_cast<uint64_t>(NSProcessInfo.processInfo.systemUptime * 1000);
    const uint8_t held = ModifierBits(CGEventGetFlags(event));
    constexpr Key keys[] = {Key::Ctrl, Key::Alt, Key::Shift, Key::Meta};
    constexpr uint8_t bits[] = {kModCtrl, kModAlt, kModShift, kModMeta};
    if (type != kCGEventFlagsChanged) {
        // Recover after a lost release or starting while a modifier is held.
        tracker.Reconcile(held);
    }
    for (int i = 0; i < 4; ++i) {
        if ((tracker.Modifiers() & bits[i]) != (held & bits[i])) {
            [self present:tracker.OnKey({keys[i], 0, (held & bits[i]) != 0}, now)];
        }
    }
    if (type == kCGEventFlagsChanged) {
        if (CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode) == kVK_CapsLock)
            [self present:tracker.OnKey({Key::CapsLock, 0, true}, now)];
        return;
    }
    if (type == kCGEventKeyDown || type == kCGEventKeyUp) {
        const auto key = TranslateKey(static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode)),
                                      type == kCGEventKeyDown);
        [self present:tracker.OnKey(key, now)];
        return;
    }
    tracker.CancelTap();
    if (!self.showMouse) return;
    switch (type) {
    case kCGEventLeftMouseDown: [self present:tracker.OnMouseButton(MouseGlyph::Left, now)]; break;
    case kCGEventRightMouseDown: [self present:tracker.OnMouseButton(MouseGlyph::Right, now)]; break;
    case kCGEventOtherMouseDown: {
        const int64_t button = CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber);
        if (button >= 2 && button <= 4)
            [self present:tracker.OnMouseButton(button == 2 ? MouseGlyph::Middle :
                (button == 3 ? MouseGlyph::Back : MouseGlyph::Forward), now)];
        break;
    }
    case kCGEventScrollWheel: {
        // Ignore momentum: show physical gestures rather than the continued glide.
        if (CGEventGetIntegerValueField(event, kCGScrollWheelEventMomentumPhase) != 0) break;
        const int vertical = static_cast<int>(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis1));
        const int horizontal = static_cast<int>(CGEventGetIntegerValueField(event, kCGScrollWheelEventDeltaAxis2));
        if (vertical) [self present:tracker.OnWheel(vertical, false, now)];
        if (horizontal) [self present:tracker.OnWheel(-horizontal, true, now)];
        break;
    }
    default: break;
    }
}
- (void)startInput {
    if (self.paused || eventTap || !CGPreflightListenEventAccess()) return;
    CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventKeyUp) |
        CGEventMaskBit(kCGEventFlagsChanged);
    if (self.showMouse) mask |= CGEventMaskBit(kCGEventLeftMouseDown) | CGEventMaskBit(kCGEventRightMouseDown) |
        CGEventMaskBit(kCGEventOtherMouseDown) | CGEventMaskBit(kCGEventScrollWheel);
    eventTap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly,
                                mask, InputCallback, (__bridge void*)self);
    if (!eventTap) return;
    eventSource = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!eventSource) { [self stopInput]; return; }
    CFRunLoopAddSource(CFRunLoopGetMain(), eventSource, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    self.statusItem.button.appearsDisabled = NO;
}
- (void)stopInput {
    if (eventSource) {
        CFRunLoopRemoveSource(CFRunLoopGetMain(), eventSource, kCFRunLoopCommonModes);
        CFRelease(eventSource);
        eventSource = nullptr;
    }
    if (eventTap) {
        CGEventTapEnable(eventTap, false);
        CFMachPortInvalidate(eventTap);
        CFRelease(eventTap);
        eventTap = nullptr;
    }
    tracker = InputTracker{};
    [self.overlay hide];
}
- (NSMenuItem*)item:(NSString*)title action:(SEL)action {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:@""];
    item.target = self;
    return item;
}
- (void)rebuildMenu {
    NSMenu* menu = [NSMenu new];
    menu.delegate = self;
    NSMenuItem* status = [self item:@"" action:nil];
    status.tag = 100;
    [menu addItem:status];
    [menu addItem:[self item:[self text:@"입력 모니터링 설정…" english:@"Input Monitoring settings…"] action:@selector(showPermissionHelp:)]];
    [menu addItem:NSMenuItem.separatorItem];
    NSMenuItem* pause = [self item:[self text:@"일시정지" english:@"Pause"] action:@selector(togglePause:)];
    pause.state = self.paused ? NSControlStateValueOn : NSControlStateValueOff;
    [menu addItem:pause];
    NSMenuItem* mouse = [self item:[self text:@"마우스 표시" english:@"Show mouse"] action:@selector(toggleMouse:)];
    mouse.state = self.showMouse ? NSControlStateValueOn : NSControlStateValueOff;
    [menu addItem:mouse];
    NSMenuItem* languages = [self item:[self text:@"언어" english:@"Language"] action:nil];
    languages.submenu = [NSMenu new];
    NSMenuItem* korean = [self item:@"한국어" action:@selector(setKorean:)];
    korean.state = self.english ? NSControlStateValueOff : NSControlStateValueOn;
    NSMenuItem* english = [self item:@"English" action:@selector(chooseEnglish:)];
    english.state = self.english ? NSControlStateValueOn : NSControlStateValueOff;
    [languages.submenu addItem:korean];
    [languages.submenu addItem:english];
    [menu addItem:languages];
    [menu addItem:[self item:[self text:@"정보" english:@"About"] action:@selector(showAbout:)]];
    [menu addItem:NSMenuItem.separatorItem];
    [menu addItem:[self item:[self text:@"종료" english:@"Quit"] action:@selector(quit:)]];
    self.statusItem.menu = menu;
    [self updateStatus];
}
- (void)updateStatus {
    [self.statusItem.menu itemWithTag:100].title = self.paused ? [self text:@"일시정지됨" english:@"Paused"] :
        (eventTap ? [self text:@"입력 표시 중" english:@"Showing input"] :
                    [self text:@"입력 모니터링 권한 필요" english:@"Input Monitoring permission needed"]);
    self.statusItem.button.appearsDisabled = self.paused || !eventTap;
}
- (void)menuWillOpen:(NSMenu*)menu {
    (void)menu;
    if (eventTap && !CGPreflightListenEventAccess()) [self stopInput];
    [self startInput];
    [self updateStatus];
}
- (void)togglePause:(id)sender {
    (void)sender;
    self.paused = !self.paused;
    [self stopInput];
    [self startInput];
    [self rebuildMenu];
}
- (void)toggleMouse:(id)sender {
    (void)sender;
    self.showMouse = !self.showMouse;
    [self stopInput];
    [self startInput];
    [self rebuildMenu];
}
- (void)setLanguage:(BOOL)english {
    self.english = english;
    [NSUserDefaults.standardUserDefaults setObject:(english ? @"en" : @"ko") forKey:@"DisplayLanguage"];
    [self.overlay hide];
    [self rebuildMenu];
}
- (void)setKorean:(id)sender { (void)sender; [self setLanguage:NO]; }
- (void)chooseEnglish:(id)sender { (void)sender; [self setLanguage:YES]; }
- (void)showPermissionHelp:(id)sender {
    (void)sender;
    NSAlert* alert = [NSAlert new];
    alert.messageText = [self text:@"입력 모니터링 허용" english:@"Allow Input Monitoring"];
    alert.informativeText = [self text:
        @"시스템 설정 → 개인정보 보호 및 보안 → 입력 모니터링에서 Chojema InputLens를 허용해 주세요. 목록에 없으면 + 버튼으로 응용 프로그램 폴더의 앱을 추가하세요. 허용 후 앱을 다시 실행하세요. 입력 내용은 저장하거나 전송하지 않습니다."
        english:@"Allow Chojema InputLens in System Settings → Privacy & Security → Input Monitoring. If it is missing, use + to add the app from Applications. Quit and reopen the app after granting permission. Input is never stored or sent anywhere."];
    [alert addButtonWithTitle:[self text:@"설정 열기" english:@"Open Settings"]];
    [alert addButtonWithTitle:[self text:@"나중에" english:@"Later"]];
    [NSApp activateIgnoringOtherApps:YES];
    if ([alert runModal] == NSAlertFirstButtonReturn) {
        CGRequestListenEventAccess();
        [NSWorkspace.sharedWorkspace openURL:[NSURL URLWithString:@"x-apple.systempreferences:com.apple.preference.security?Privacy_ListenEvent"]];
    }
}
- (void)showAbout:(id)sender {
    (void)sender;
    NSAlert* alert = [NSAlert new];
    NSString* version = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleShortVersionString"];
    alert.messageText = [@"Chojema InputLens " stringByAppendingString:version ? version : @"0.1.0"];
    alert.informativeText = [[self text:@"입력한 내용을 저장하거나 전송하지 않습니다." english:@"Input is never stored or sent anywhere."]
        stringByAppendingString:@"\n\nCopyright © 2026 Chulwoo Park"];
    [alert addButtonWithTitle:[self text:@"확인" english:@"OK"]];
    [NSApp activateIgnoringOtherApps:YES];
    [alert runModal];
}
- (void)quit:(id)sender { (void)sender; [NSApp terminate:nil]; }
- (void)applicationWillTerminate:(NSNotification*)notification {
    (void)notification;
    [self stopInput];
    [NSWorkspace.sharedWorkspace.notificationCenter removeObserver:self];
}
@end

#ifndef INPUTLENS_TEST
int main() {
    @autoreleasepool {
        NSApplication* app = NSApplication.sharedApplication;
        NSString* identifier = NSBundle.mainBundle.bundleIdentifier;
        for (NSRunningApplication* running in [NSRunningApplication runningApplicationsWithBundleIdentifier:identifier]) {
            if (running.processIdentifier != NSProcessInfo.processInfo.processIdentifier) return 0;
        }
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        AppDelegate* delegate = [AppDelegate new];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
#endif
