#include "config.hh"

#include <cstdlib>
#include <fstream>
#include <regex>

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

std::string Config::ExpandEnvironmentVariables(const std::string& input) {
  std::regex env_var_pattern("\\$(\\w+(_\\w+)*)");
  std::string result = input;

  std::smatch match;
  std::string::const_iterator search_start(result.cbegin());

  while (
      std::regex_search(search_start, result.cend(), match, env_var_pattern)) {
    const std::string var_name = match[1].str();
    const char* env_value = std::getenv(var_name.c_str());

    if (env_value) {
      const size_t pos = match.position(0) + (search_start - result.cbegin());
      const size_t length = match[0].length();
      result.replace(pos, length, env_value);

      search_start = result.cbegin() + pos + strlen(env_value);
    } else {
      search_start = match.suffix().first;
    }
  }

  return result;
}

}  // namespace utils
}  // namespace soir
