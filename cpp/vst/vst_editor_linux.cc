#include <absl/log/log.h>

namespace soir {
namespace vst {

void* CreateEditorWindow(int width, int height, const char* title) {
  LOG(WARNING) << "VST editor windows are not supported on Linux";
  return nullptr;
}

void ResizeEditorWindow(void* view, int width, int height) {}

void ShowEditorWindow(void* view) {}

void CloseEditorWindow(void* view) {}

void PumpEvents() {}

void GetEditorWindowSize(void* view, int* width, int* height) {
  *width = 0;
  *height = 0;
}

}  // namespace vst
}  // namespace soir
