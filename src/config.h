#ifndef SOIR_CONFIG_H
#define SOIR_CONFIG_H

#include <memory>
#include <string>
#include <vector>

#include <yaml-cpp/yaml.h>

namespace soir {

// Config provides a wrapper to access settings from configuration
// files. It uses YAML for now as it is convenient to have something
// that supports comments. It's a leaky abstraction in the sense that
// we don't try to hide YAML from the interface.
class Config {
public:
  // Let this private to make it explicit that this class needs to be
  // instantiated using LoadFromPath.
  explicit Config(const YAML::Node &node);

  // Create a new config from a path.
  static std::unique_ptr<Config> LoadFromPath(const std::string &path);

  // Get a setting from the config, fallback to defaults if there is
  // no such setting defined.
  template <typename T> T Get(const std::string &location, const T &def) const {
    return GetChildNode(location).as<T>(def);
  }

  // Similar but uses default-constructor as a default in case the
  // setting is not defined in the config file.
  template <typename T> T Get(const std::string &location) const {
    return Get(location, T());
  }

  // Get a config from a part of a config.
  std::unique_ptr<Config> GetConfig(const std::string &location) const;

  // Get a list of configs from a part of a config.
  std::vector<std::unique_ptr<Config>>
  GetConfigs(const std::string &location) const;

private:
  // Helper to get a YAML node from a location.
  YAML::Node GetChildNode(const std::string &location) const;

  // The node this Config is wrapping.
  YAML::Node node_;
};

} // namespace soir

#endif // SOIR_CONFIG_H
