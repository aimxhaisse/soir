#pragma once

#include <yaml-cpp/yaml.h>

#include <memory>
#include <string>
#include <vector>

#include "Status.hh"

namespace maethstro {

class ConfigParser {
 public:
  explicit ConfigParser(const YAML::Node& node);

  static StatusOr<std::unique_ptr<ConfigParser>> LoadFromPath(
      const std::string& path);

  static StatusOr<std::unique_ptr<ConfigParser>> LoadFromString(
      const std::string& content);

  template <typename T>
  T Get(const std::string& location, const T& def) const {
    return GetChildNode(location).as<T>(def);
  }

  template <typename T>
  T Get(const std::string& location) const {
    return Get(location, T());
  }

  std::unique_ptr<ConfigParser> GetConfig(const std::string& location) const;

  std::vector<std::unique_ptr<ConfigParser>> GetConfigs(
      const std::string& location) const;

 private:
  YAML::Node GetChildNode(const std::string& location) const;

  YAML::Node node_;
};

}  // namespace maethstro
