#include "vst/vst_plug_frame.hh"

namespace soir {
namespace vst {

using namespace Steinberg;

tresult PLUGIN_API HostPlugFrame::resizeView(IPlugView* view,
                                             ViewRect* new_size) {
  if (view && new_size) {
    view->onSize(new_size);
  }
  return kResultOk;
}

tresult PLUGIN_API HostPlugFrame::queryInterface(const TUID iid, void** obj) {
  if (FUnknownPrivate::iidEqual(iid, IPlugFrame::iid) ||
      FUnknownPrivate::iidEqual(iid, FUnknown::iid)) {
    *obj = static_cast<IPlugFrame*>(this);
    addRef();
    return kResultOk;
  }
  return QueryPlatformInterface(iid, obj);
}

uint32 PLUGIN_API HostPlugFrame::addRef() { return ++ref_count_; }

uint32 PLUGIN_API HostPlugFrame::release() {
  auto count = --ref_count_;
  if (count == 0) {
    delete this;
  }
  return count;
}

tresult HostPlugFrame::QueryPlatformInterface(const TUID iid, void** obj) {
  *obj = nullptr;
  return kNoInterface;
}

}  // namespace vst
}  // namespace soir
