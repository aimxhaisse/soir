#pragma once

#include <absl/status/status.h>

#include "agent/controller_watcher.hh"
#include "agent/file_watcher.hh"
#include "agent/subscriber.hh"
#include "utils/config.hh"

namespace neon {
namespace agent {

class Agent {
 public:
  Agent();
  ~Agent();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  std::unique_ptr<proto::Neon::Stub> neon_stub_;
  std::unique_ptr<ControllerWatcher> controller_watcher_;
  std::unique_ptr<FileWatcher> file_watcher_;
  std::unique_ptr<Subscriber> subscriber_;
};

}  // namespace agent
}  // namespace neon
