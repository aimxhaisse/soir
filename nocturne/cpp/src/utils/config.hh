#pragma once

#include <absl/status/statusor.h>

#include <nlohmann/json.hpp>
#include <string>

namespace soir {
namespace utils {

class Config {
 public:
  explicit Config(const std::string& json_str);

  template <typename T>
  T Get(const std::string& path) const {
    return GetNode(path).get<T>();
  }

  static Config FromJson(const nlohmann::json& json);
  static absl::StatusOr<Config> FromPath(const std::string& path);

 private:
  struct FromJsonTag {};
  Config(const nlohmann::json& json, FromJsonTag);

  nlohmann::json GetNode(const std::string& path) const;

  nlohmann::json data_;
};

}  // namespace utils
}  // namespace soir

namespace nlohmann {

// Needed to be able to Get<Config>()
template <>
struct adl_serializer<soir::utils::Config> {
  static soir::utils::Config from_json(const json& j) {
    return soir::utils::Config::FromJson(j);
  }
};

}  // namespace nlohmann
