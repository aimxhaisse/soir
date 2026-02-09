#import <Cocoa/Cocoa.h>

#include <absl/log/log.h>

#include "vst/vst_plugin.hh"

// Objective-C class must be at global scope.
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

namespace {

bool app_initialized_ = false;

void EnsureAppInitialized() {
  if (app_initialized_) {
    return;
  }
  app_initialized_ = true;
  [NSApplication sharedApplication];
  [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
}

}  // namespace

void* CreateEditorWindow(int width, int height, const char* title) {
  EnsureAppInitialized();
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

void ResizeEditorWindow(void* view, int width, int height) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view;
    NSWindow* window = [nsview window];
    NSRect frame = [window frame];
    NSRect content = NSMakeRect(0, 0, width, height);
    NSRect new_frame = [window frameRectForContentRect:content];
    new_frame.origin = frame.origin;
    [window setFrame:new_frame display:YES];
    [window center];
  }
}

void ShowEditorWindow(void* view) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view;
    NSWindow* window = [nsview window];
    [NSApp activateIgnoringOtherApps:YES];
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

void PumpEvents() {
  @autoreleasepool {
    NSEvent* event;
    while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES])) {
      [NSApp sendEvent:event];
    }
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
