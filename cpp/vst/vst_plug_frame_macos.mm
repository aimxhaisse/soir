#include "vst/vst_plug_frame.hh"

namespace soir {
namespace vst {

std::unique_ptr<HostPlugFrame> CreateHostPlugFrame() {
  return std::make_unique<HostPlugFrame>();
}

}  // namespace vst
}  // namespace soir
