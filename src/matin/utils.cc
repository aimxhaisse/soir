#include "utils.hh"

namespace maethstro {
namespace matin {
namespace utils {

void InitContext(grpc::ClientContext* context, const std::string& user) {
  context->AddMetadata("user", user);
}

}  // namespace utils
}  // namespace matin
}  // namespace maethstro
