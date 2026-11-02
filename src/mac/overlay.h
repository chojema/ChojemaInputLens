#pragma once
#import <Cocoa/Cocoa.h>
#include "input_tracker.h"

@interface InputOverlay : NSObject
- (void)showPrefix:(NSString*)prefix glyph:(inputlens::MouseGlyph)glyph count:(int)count;
- (void)hide;
@end
