#include <fstream>

#include "utils.hh"

namespace maethstro {
namespace common {
namespace utils {

absl::StatusOr<std::string> GetFileContents(const std::string& filename) {
  std::ifstream file(filename);
  if (!file.is_open()) {
    return absl::NotFoundError("File not found: " + filename);
  }

  return std::string(std::istreambuf_iterator<char>(file),
                     std::istreambuf_iterator<char>());
}

}  // namespace utils
}  // namespace common
}  // namespace maethstro
