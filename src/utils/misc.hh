
#pragma once

#include <absl/status/statusor.h>

namespace soir {
namespace utils {

absl::StatusOr<std::string> GetFileContents(const std::string& filename);

}  // namespace utils
}  // namespace soir
