#include <sstream>

#include "utils.h"

namespace soir {
namespace utils {

std::vector<std::string> StringSplit(const std::string &str, char delim) {
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  std::string item;

  while (getline(ss, item, delim)) {
    if (!item.empty()) {
      tokens.push_back(item);
    }
  }

  return tokens;
}

uint32_t SwapEndian(uint32_t val) {
  return (uint32_t)((((uint32_t)(val) & (uint32_t)0x000000FFU) << 24) |
                    (((uint32_t)(val) & (uint32_t)0x0000FF00U) << 8) |
                    (((uint32_t)(val) & (uint32_t)0x00FF0000U) >> 8) |
                    (((uint32_t)(val) & (uint32_t)0xFF000000U) >> 24));
}

} // namespace utils
} // namespace soir
