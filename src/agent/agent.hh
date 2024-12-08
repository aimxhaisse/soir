#pragma once

#include <absl/status/status.h>

#include "agent/controller_watcher.hh"
#include "agent/file_watcher.hh"
#include "agent/subscriber.hh"
#include "utils/config.hh"

namespace soir {
namespace agent {

// Agent is a small class that watches incoming events and sends them
// to the engine via gRPC calls: it can in theory run on another
// machine than the engine for this reason.
//
// - watches for a directory with live code files
// - watches for incoming MIDI events from controllers
class Agent {
 public:
  Agent();
  ~Agent();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  std::unique_ptr<proto::Soir::Stub> soir_stub_;
  std::unique_ptr<ControllerWatcher> controller_watcher_;
  std::unique_ptr<FileWatcher> file_watcher_;
  std::unique_ptr<Subscriber> subscriber_;
};

}  // namespace agent
}  // namespace soir
