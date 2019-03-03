#include <algorithm>

#include <glog/logging.h>

#include "config.h"
#include "utils.h"

namespace soir {

Config::Config(const YAML::Node &node) : node_(node) {}

std::unique_ptr<Config> Config::LoadFromPath(const std::string &path) {
  try {
    YAML::Node node = YAML::LoadFile(path);
    return std::make_unique<Config>(node);
  } catch (const std::exception &error) {
    LOG(WARNING) << "Unable to load '" << path << "': " << error.what() << ".";
  }
  return std::unique_ptr<Config>(nullptr);
}

std::unique_ptr<Config> Config::GetConfig(const std::string &location) const {
  return std::make_unique<Config>(GetChildNode(location));
}

std::vector<std::unique_ptr<Config>>
Config::GetConfigs(const std::string &location) const {
  std::vector<std::unique_ptr<Config>> configs;
  for (auto node : GetChildNode(location)) {
    configs.push_back(std::make_unique<Config>(node));
  }
  return configs;
}

YAML::Node Config::GetChildNode(const std::string &location) const {
  auto keys = utils::StringSplit(location, '.');

  // This is probably full of copies, which might be a problem at some
  // point if we regularly query this in live situations. I haven't
  // found a way to avoid such copies using the current API of YAML
  // cpp, might be worth spending more time on this.
  YAML::Node current = YAML::Clone(node_);
  for (const auto &key : keys) {
    current = current[key];
  }

  return current;
}

} // namespace soir
