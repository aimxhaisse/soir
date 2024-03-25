
#pragma once

#include <absl/status/statusor.h>

namespace neon {
namespace utils {

absl::StatusOr<std::string> GetFileContents(const std::string& filename);

}  // namespace utils
}  // namespace neon
