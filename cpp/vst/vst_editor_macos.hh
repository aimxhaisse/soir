#pragma once

#include "vst/vst_editor.hh"

namespace soir {
namespace vst {

class MacOsEditorWindow : public EditorWindow {
 public:
  explicit MacOsEditorWindow(void* retained_view);
  ~MacOsEditorWindow() override;

  void* NativeHandle() override;
  void Resize(int width, int height) override;
  void Show() override;
  void Hide() override;
  void GetSize(int* width, int* height) override;

 private:
  void* view_;  // Retained NSView*.
};

}  // namespace vst
}  // namespace soir
