#pragma once

#include <memory>

namespace soir {
namespace vst {

class EditorWindow {
 public:
  static std::unique_ptr<EditorWindow> Create(int width, int height,
                                              const char* title);
  static void PumpEvents();

  virtual ~EditorWindow() = default;

  virtual void* NativeHandle() = 0;
  virtual void Resize(int width, int height) = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void GetSize(int* width, int* height) = 0;
};

}  // namespace vst
}  // namespace soir
