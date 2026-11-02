#pragma once
#import <Cocoa/Cocoa.h>
#include "input_tracker.h"

inputlens::KeyEvent TranslateKey(CGKeyCode code, bool down);
uint8_t ModifierBits(CGEventFlags flags);
NSString* DisplayPrefix(const inputlens::Display& display, bool english);
