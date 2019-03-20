#ifndef SOIR_UTILS_H
#define SOIR_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace soir {
namespace utils {

std::vector<std::string> StringSplit(const std::string &str, char delim);

uint32_t SwapEndian(uint32_t val);

} // namespace utils
} // namespace soir

#endif
