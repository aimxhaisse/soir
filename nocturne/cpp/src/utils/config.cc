#include "config.hh"

#include <fstream>

namespace soir {
namespace utils {

Config::Config(const std::string& json_str)
    : data_(nlohmann::json::parse(json_str)) {}

Config::Config(const nlohmann::json& json, FromJsonTag) : data_(json) {}

Config Config::FromJson(const nlohmann::json& json) {
  return Config(json, FromJsonTag{});
}

absl::StatusOr<Config> Config::FromPath(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return absl::NotFoundError("Failed to open config file: " + path);
  }

  nlohmann::json json;
  try {
    file >> json;
  } catch (const nlohmann::json::exception& e) {
    return absl::InvalidArgumentError("Failed to parse config file: " +
                                      std::string(e.what()));
  }

  return Config(json, FromJsonTag{});
}

nlohmann::json Config::GetNode(const std::string& path) const {
  if (path.empty()) {
    return data_;
  }

  auto node = data_;
  size_t start = 0;

  while (start < path.size()) {
    size_t dot = path.find('.', start);
    std::string key = (dot == std::string::npos)
                          ? path.substr(start)
                          : path.substr(start, dot - start);

    node = node.at(key);

    if (dot == std::string::npos) {
      break;
    }
    start = dot + 1;
  }

  return node;
}

}  // namespace utils
}  // namespace soir
