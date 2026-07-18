#include "vst/vst_plugin.hh"

namespace soir {
namespace vst {

Steinberg::tresult VstPlugin::AttachEditorView(Steinberg::IPlugView* view,
                                               void* parent_window) {
  return view->attached(parent_window,
                        Steinberg::kPlatformTypeNSView);
}

}  // namespace vst
}  // namespace soir
