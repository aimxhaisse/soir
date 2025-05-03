#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/str_split.h>
#include <algorithm>
#include <cstdlib>
#include <regex>

#include "config.hh"

namespace soir {
namespace utils {

Config::Config(const YAML::Node& node) : node_(node) {}

absl::StatusOr<std::unique_ptr<Config>> Config::LoadFromPath(
    const std::string& path) {
  LOG(INFO) << "Loading configuration file " << path;

  try {
    YAML::Node node = YAML::LoadFile(path);
    return std::make_unique<Config>(node);
  } catch (const std::exception& error) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unable to load '", path, "': ", error.what(), "."));
  }
}

absl::StatusOr<std::unique_ptr<Config>> Config::LoadFromString(
    const std::string& content) {
  try {
    YAML::Node node = YAML::Load(content);
    return std::make_unique<Config>(node);
  } catch (const std::exception& error) {
    return absl::InvalidArgumentError(
        absl::StrCat("Unable to load configuration: ", error.what(), "."));
  }
}

template <>
std::string Config::Get(const std::string& location, const std::string& def) const {
  std::string value = GetChildNode(location).as<std::string>(def);
  return ExpandEnvironmentVariables(value);
}

template <>
std::string Config::Get(const std::string& location) const {
  return Get<std::string>(location, std::string());
}

std::string Config::ExpandEnvironmentVariables(const std::string& input) const {
  std::regex env_var_pattern("\\$(\\w+(_\\w+)*)");
  std::string result = input;
  
  std::smatch match;
  std::string::const_iterator search_start(result.cbegin());
  
  while (std::regex_search(search_start, result.cend(), match, env_var_pattern)) {
    const std::string var_name = match[1].str();
    const char* env_value = std::getenv(var_name.c_str());
    
    if (env_value) {
      // Replace the environment variable reference with its value
      const size_t pos = match.position(0) + (search_start - result.cbegin());
      const size_t length = match[0].length();
      result.replace(pos, length, env_value);
      
      // Update search start position accounting for the replacement
      search_start = result.cbegin() + pos + strlen(env_value);
    } else {
      // Keep the original reference if the environment variable is not set
      search_start = result.cbegin() + match.position(0) + match[0].length();
    }
  }
  
  return result;
}

std::unique_ptr<Config> Config::GetConfig(const std::string& location) const {
  return std::make_unique<Config>(GetChildNode(location));
}

std::vector<std::unique_ptr<Config>> Config::GetConfigs(
    const std::string& location) const {
  std::vector<std::unique_ptr<Config>> configs;
  for (auto node : GetChildNode(location)) {
    configs.push_back(std::make_unique<Config>(node));
  }
  return configs;
}

YAML::Node Config::GetChildNode(const std::string& location) const {
  auto keys = absl::StrSplit(location, '.');

  // This is probably full of copies, which might be a problem at some
  // point if we regularly query this in live situations. I haven't
  // found a way to avoid such copies using the current API of YAML
  // cpp, might be worth spending more time on this.
  YAML::Node current = YAML::Clone(node_);
  for (const auto& key : keys) {
    current = current[std::string(key)];
  }

  return current;
}

}  // namespace utils
}  // namespace soir
