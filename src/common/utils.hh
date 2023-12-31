#pragma once

#include <absl/status/statusor.h>

namespace maethstro {
namespace common {
namespace utils {

absl::StatusOr<std::string> GetFileContents(const std::string& filename);

}  // namespace utils
}  // namespace common
}  // namespace maethstro
