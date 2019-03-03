#ifndef SOIR_UTILS_H
#define SOIR_UTILS_H

#include <string>
#include <vector>

namespace soir {
namespace utils {

// Splits a string from a delimiter.
std::vector<std::string> StringSplit(const std::string &str, char delim);

} // namespace utils
} // namespace soir

#endif
