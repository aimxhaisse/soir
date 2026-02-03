#import <Cocoa/Cocoa.h>

#include <absl/log/log.h>

#include "vst/vst_plugin.hh"

// Objective-C class must be at global scope
@interface VstEditorWindow : NSWindow
@end

@implementation VstEditorWindow

- (BOOL)canBecomeKeyWindow {
  return YES;
}

- (BOOL)canBecomeMainWindow {
  return YES;
}

@end

namespace soir {
namespace vst {

void* CreateEditorWindow(int width, int height, const char* title) {
  @autoreleasepool {
    NSRect frame = NSMakeRect(0, 0, width, height);

    VstEditorWindow* window = [[VstEditorWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    [window setTitle:[NSString stringWithUTF8String:title]];
    [window center];
    [window setReleasedWhenClosed:NO];

    return (__bridge_retained void*)[window contentView];
  }
}

void ShowEditorWindow(void* view) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view;
    NSWindow* window = [nsview window];
    [window makeKeyAndOrderFront:nil];
  }
}

void CloseEditorWindow(void* view) {
  @autoreleasepool {
    NSView* nsview = (__bridge_transfer NSView*)view;
    NSWindow* window = [nsview window];
    [window close];
  }
}

void GetEditorWindowSize(void* view, int* width, int* height) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view;
    NSRect frame = [nsview frame];
    *width = static_cast<int>(frame.size.width);
    *height = static_cast<int>(frame.size.height);
  }
}

}  // namespace vst
}  // namespace soir
