#include "Utils.hh"

#include <glog/logging.h>

#include <sstream>

namespace maethstro {
namespace utils {

std::vector<std::string> StringSplit(const std::string& str, char delim) {
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

}  // namespace utils
}  // namespace maethstro
