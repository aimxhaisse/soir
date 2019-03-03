#include <sstream>

#include "utils.h"

namespace soir {
namespace utils {

std::vector<std::string> StringSplit(const std::string &str, char delim) {
  std::stringstream ss(str);
  std::vector<std::string> tokens;
  std::string item;

  while (getline(ss, item, delim)) {
    tokens.push_back(item);
  }

  return tokens;
}

} // namespace utils
} // namespace soir
