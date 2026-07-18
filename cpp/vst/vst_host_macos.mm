#include "vst/vst_host_macos.hh"

namespace soir {
namespace vst {

HostContext* CreateHostContext() { return new MacOSHostContext(); }

}  // namespace vst
}  // namespace soir
