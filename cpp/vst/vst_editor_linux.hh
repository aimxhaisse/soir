#pragma once

#include <X11/Xlib.h>

#include <mutex>
#include <vector>

#include "vst/vst_editor.hh"

namespace soir {
namespace vst {

class LinuxEditorWindow : public EditorWindow {
 public:
  LinuxEditorWindow(Display* display, Window window, Atom wm_delete_window);
  ~LinuxEditorWindow() override;

  void* NativeHandle() override;
  void Resize(int width, int height) override;
  void Show() override;
  void Hide() override;
  void GetSize(int* width, int* height) override;

  // Called by EditorWindow::PumpEvents() to process pending X11 events on all
  // live windows. Handles WM_DELETE_WINDOW by hiding the window.
  static void PumpAll();

 private:
  Display* display_;
  Window window_;
  Atom wm_delete_window_;

  static std::mutex registry_mutex_;
  static std::vector<LinuxEditorWindow*> registry_;
};

}  // namespace vst
}  // namespace soir
