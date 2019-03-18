#ifndef SOIR_UTILS_H
#define SOIR_UTILS_H

#include <cstdint>
#include <string>
#include <vector>

namespace soir {
namespace utils {

// Splits a string from a delimiter.
std::vector<std::string> StringSplit(const std::string &str, char delim);

// Swap endianness.
uint32_t SwapEndian(uint32_t val);

} // namespace utils
} // namespace soir

#endif
