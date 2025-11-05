#pragma once

#include <memory>
#include <string>

#include "absl/status/status.h"
#include "utils/config.hh"

namespace soir {

class Soir {
 public:
  absl::Status Init(const std::string& config_path);
  absl::Status Start();
  absl::Status Stop();
  absl::Status UpdateCode(const std::string& code);

 private:
  std::unique_ptr<utils::Config> config_;
  bool initialized_ = false;
  bool running_ = false;
};

}  // namespace soir
