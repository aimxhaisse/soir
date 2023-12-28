#pragma once

#include <absl/status/statusor.h>

namespace maethstro {
namespace utils {

absl::StatusOr<std::string> GetFileContents(const std::string& filename);

}  // namespace utils
}  // namespace maethstro
