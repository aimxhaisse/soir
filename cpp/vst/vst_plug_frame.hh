#pragma once

#include <atomic>
#include <memory>

#include "pluginterfaces/gui/iplugview.h"

namespace soir {
namespace vst {

// Minimal IPlugFrame implementation. The VST3 spec requires the host to call
// IPlugView::setFrame() before IPlugView::attached(); several plug-in
// frameworks assert on this and refuse to attach when no frame is provided.
// resizeView() is the plug-in's request to grow/shrink its window — we honor
// it by acknowledging the new size on the view; the parent window keeps its
// initial size since the editor is hosted in a fixed-size window.
//
// Platform-specific extensions (e.g. Steinberg::Linux::IRunLoop) are provided
// by derived classes in the per-platform translation units.
class HostPlugFrame : public Steinberg::IPlugFrame {
 public:
  HostPlugFrame() = default;

  Steinberg::tresult PLUGIN_API resizeView(
      Steinberg::IPlugView* view, Steinberg::ViewRect* new_size) override;

  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,
                                               void** obj) override;
  Steinberg::uint32 PLUGIN_API addRef() override;
  Steinberg::uint32 PLUGIN_API release() override;

 protected:
  // Hook for platform-specific interfaces (e.g. Linux::IRunLoop).
  virtual Steinberg::tresult QueryPlatformInterface(const Steinberg::TUID iid,
                                                    void** obj);

 private:
  std::atomic<Steinberg::int32> ref_count_{1};
};

// Factory — returns the appropriate concrete type for the platform.
std::unique_ptr<HostPlugFrame> CreateHostPlugFrame();

}  // namespace vst
}  // namespace soir
