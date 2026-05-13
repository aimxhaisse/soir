#pragma once

#include <memory>

namespace soir {
namespace vst {

class EditorWindow {
 public:
  // One-time platform-level setup the rest of the VST host depends on. On
  // Linux this installs an XLib error handler that swallows non-fatal errors
  // from plugin code (e.g. X_ShmDetach during plugin teardown), which would
  // otherwise call exit(1). Must run before any plugin instance is created —
  // VstHost::Init() calls this before scanning.
  static void InitPlatform();

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
