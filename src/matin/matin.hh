#pragma once

#include <absl/status/status.h>
#include <regex>

#include "common/config.hh"
#include "file_watcher.hh"
#include "subscriber.hh"

namespace maethstro {
namespace matin {

struct MatinSettings {
  std::string username;
  std::string directory;
  std::string midi_grpc_host;
  int midi_grpc_port;
};

class Matin {
 public:
  Matin();
  ~Matin();

  absl::Status Init(const Config& config);
  absl::Status Start();
  absl::Status Stop();

 private:
  MatinSettings settings_;
  std::unique_ptr<matin::Subscriber> subscriber_;
  std::unique_ptr<FileWatcher> file_watcher_;
};

}  // namespace matin
}  // namespace maethstro
