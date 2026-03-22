#import <Cocoa/Cocoa.h>

#include "vst/vst_editor_macos.hh"

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

MacOsEditorWindow::MacOsEditorWindow(void* retained_view)
    : view_(retained_view) {}

MacOsEditorWindow::~MacOsEditorWindow() {
  @autoreleasepool {
    NSView* nsview = (__bridge_transfer NSView*)view_;
    [[nsview window] close];
  }
}

void* MacOsEditorWindow::NativeHandle() {
  return view_;
}

void MacOsEditorWindow::Resize(int width, int height) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view_;
    NSWindow* window = [nsview window];
    NSRect frame = [window frame];
    NSRect content = NSMakeRect(0, 0, width, height);
    NSRect new_frame = [window frameRectForContentRect:content];
    new_frame.origin = frame.origin;
    [window setFrame:new_frame display:YES];
    [window center];
  }
}

void MacOsEditorWindow::Show() {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view_;
    NSWindow* window = [nsview window];
    [NSApp activateIgnoringOtherApps:YES];
    [window makeKeyAndOrderFront:nil];
  }
}

void MacOsEditorWindow::GetSize(int* width, int* height) {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view_;
    NSRect frame = [nsview frame];
    *width = static_cast<int>(frame.size.width);
    *height = static_cast<int>(frame.size.height);
  }
}

void MacOsEditorWindow::Hide() {
  @autoreleasepool {
    NSView* nsview = (__bridge NSView*)view_;
    [[nsview window] orderOut:nil];
  }
}

std::unique_ptr<EditorWindow> EditorWindow::Create(int width, int height,
                                                   const char* title) {
  @autoreleasepool {
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, width, height);
    VstEditorWindow* window = [[VstEditorWindow alloc]
        initWithContentRect:frame
                  styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable
                    backing:NSBackingStoreBuffered
                      defer:NO];

    [window setTitle:[NSString stringWithUTF8String:title]];
    [window center];
    [window setReleasedWhenClosed:NO];

    void* retained_view = (__bridge_retained void*)[window contentView];
    return std::make_unique<MacOsEditorWindow>(retained_view);
  }
}

void EditorWindow::PumpEvents() {
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

}  // namespace vst
}  // namespace soir
