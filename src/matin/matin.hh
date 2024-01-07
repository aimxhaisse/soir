#pragma once

#include <absl/status/status.h>

#include "common/config.hh"
#include "controller_watcher.hh"
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
  std::unique_ptr<proto::Midi::Stub> midi_stub_;
  std::unique_ptr<ControllerWatcher> controller_watcher_;
  std::unique_ptr<FileWatcher> file_watcher_;
  std::unique_ptr<matin::Subscriber> subscriber_;
};

}  // namespace matin
}  // namespace maethstro
