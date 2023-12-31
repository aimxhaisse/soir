#pragma once

#include <absl/status/status.h>

#include "common/config.hh"
#include "file_watcher.hh"
#include "subscriber.hh"

namespace maethstro {
namespace matin {

class Matin {
 public:
  Matin();
  ~Matin();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  std::unique_ptr<matin::Subscriber> subscriber_;
  std::unique_ptr<FileWatcher> file_watcher_;
};

}  // namespace matin
}  // namespace maethstro
