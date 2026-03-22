#include "vst/vst_editor_linux.hh"

#include <absl/log/log.h>

#include <algorithm>
#include <cstdint>
#include <mutex>

namespace soir {
namespace vst {

std::mutex LinuxEditorWindow::registry_mutex_;
std::vector<LinuxEditorWindow*> LinuxEditorWindow::registry_;

namespace {

// XInitThreads() must be called once before any other Xlib call.
void EnsureXInitThreads() {
  static std::once_flag flag;
  std::call_once(flag, [] { XInitThreads(); });
}

// Install a custom X11 error handler. During VST plugin teardown, embedded
// plugin code (e.g. yabridge running inside our process) may attempt X11
// operations on windows that are already destroyed, producing BadDrawable
// errors. These are benign: the default handler would call exit(1), which
// is the wrong behaviour for cleanup-time noise. This matches the approach
// used by GTK (gdk_error_trap_push), JUCE, and Ardour for plugin hosting.
//
// Note: X11 error handlers are called while XLib holds internal locks, so
// only async-signal-safe operations (e.g. write(2)) are safe to call here.
void EnsureErrorHandler() {
  static std::once_flag flag;
  std::call_once(flag, [] {
    XSetErrorHandler([](Display*, XErrorEvent*) -> int { return 0; });
  });
}

}  // namespace

LinuxEditorWindow::LinuxEditorWindow(Display* display, Window window,
                                     Atom wm_delete_window)
    : display_(display), window_(window), wm_delete_window_(wm_delete_window) {
  std::lock_guard<std::mutex> lock(registry_mutex_);
  registry_.push_back(this);
}

LinuxEditorWindow::~LinuxEditorWindow() {
  {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    registry_.erase(std::remove(registry_.begin(), registry_.end(), this),
                    registry_.end());
  }
  XDestroyWindow(display_, window_);
  XFlush(display_);
  XCloseDisplay(display_);
}

void* LinuxEditorWindow::NativeHandle() {
  return reinterpret_cast<void*>(static_cast<uintptr_t>(window_));
}

void LinuxEditorWindow::Resize(int width, int height) {
  XResizeWindow(display_, window_, width, height);
  XFlush(display_);
}

void LinuxEditorWindow::Show() {
  XMapRaised(display_, window_);
  XFlush(display_);
}

void LinuxEditorWindow::Hide() {
  XUnmapWindow(display_, window_);
  XFlush(display_);
}

void LinuxEditorWindow::GetSize(int* width, int* height) {
  XWindowAttributes attrs;
  if (XGetWindowAttributes(display_, window_, &attrs) != 0) {
    *width = attrs.width;
    *height = attrs.height;
  } else {
    *width = 0;
    *height = 0;
  }
}

void LinuxEditorWindow::PumpAll() {
  std::lock_guard<std::mutex> lock(registry_mutex_);
  for (auto* win : registry_) {
    XEvent event;
    while (XPending(win->display_)) {
      XNextEvent(win->display_, &event);
      if (event.type == ClientMessage &&
          static_cast<Atom>(event.xclient.data.l[0]) ==
              win->wm_delete_window_) {
        XUnmapWindow(win->display_, win->window_);
        XFlush(win->display_);
      }
    }
  }
}

std::unique_ptr<EditorWindow> EditorWindow::Create(int width, int height,
                                                   const char* title) {
  EnsureXInitThreads();
  EnsureErrorHandler();

  Display* display = XOpenDisplay(nullptr);
  if (!display) {
    LOG(ERROR) << "Failed to open X11 display";
    return nullptr;
  }

  Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);
  Window window = XCreateSimpleWindow(display, root, 0, 0, width, height, 0,
                                      BlackPixel(display, screen),
                                      BlackPixel(display, screen));

  XStoreName(display, window, title);
  XSelectInput(display, window, ExposureMask | StructureNotifyMask);
  XSetWMProtocols(display, window, &wm_delete_window, 1);
  XFlush(display);

  return std::make_unique<LinuxEditorWindow>(display, window, wm_delete_window);
}

void EditorWindow::PumpEvents() { LinuxEditorWindow::PumpAll(); }

}  // namespace vst
}  // namespace soir
