#include "vst/vst_plug_frame.hh"

namespace soir {
namespace vst {

using namespace Steinberg;

class HostPlugFrameLinux : public HostPlugFrame, public Linux::IRunLoop {
 public:
  // Linux::IRunLoop
  tresult PLUGIN_API
  registerEventHandler(Linux::IEventHandler* /*handler*/,
                       Linux::FileDescriptor /*fd*/) override {
    return kResultOk;
  }

  tresult PLUGIN_API
  unregisterEventHandler(Linux::IEventHandler* /*handler*/) override {
    return kResultOk;
  }

  tresult PLUGIN_API registerTimer(Linux::ITimerHandler* /*handler*/,
                                   Linux::TimerInterval /*ms*/) override {
    return kResultOk;
  }

  tresult PLUGIN_API
  unregisterTimer(Linux::ITimerHandler* /*handler*/) override {
    return kResultOk;
  }

  // FUnknown — forward to base so ref-counting is unified.
  tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override {
    if (FUnknownPrivate::iidEqual(iid, Linux::IRunLoop::iid)) {
      *obj = static_cast<Linux::IRunLoop*>(this);
      addRef();
      return kResultOk;
    }
    return HostPlugFrame::queryInterface(iid, obj);
  }

  uint32 PLUGIN_API addRef() override { return HostPlugFrame::addRef(); }
  uint32 PLUGIN_API release() override { return HostPlugFrame::release(); }
};

std::unique_ptr<HostPlugFrame> CreateHostPlugFrame() {
  return std::make_unique<HostPlugFrameLinux>();
}

}  // namespace vst
}  // namespace soir
