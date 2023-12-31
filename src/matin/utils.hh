#include <grpcpp/grpcpp.h>
#include <string>

namespace maethstro {
namespace matin {
namespace utils {

void InitContext(grpc::ClientContext* context, const std::string& user);

}  // namespace utils
}  // namespace matin
}  // namespace maethstro
