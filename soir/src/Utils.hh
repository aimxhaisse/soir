#pragma once

#include <vector>

#include "Status.hh"

namespace maethstro {
namespace utils {

// Split a string with a delimiter.
std::vector<std::string> StringSplit(const std::string& str, char delim);

}  // namespace utils
}  // namespace maethstro
