#pragma once

#include <string>

#include "absl/status/status.h"
#include "utils/config.hh"

namespace soir {

class Soir {
 public:
  absl::Status Init(utils::Config* config);
  absl::Status Start();
  absl::Status Stop();
  absl::Status UpdateCode(const std::string& code);

 private:
  utils::Config* config_ = nullptr;
  bool initialized_ = false;
  bool running_ = false;
};

}  // namespace soir
