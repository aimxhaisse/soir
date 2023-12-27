#include <absl/log/log.h>

#include "matin.hh"

namespace maethstro {

Matin::Matin() {
  file_watcher_ = std::make_unique<efsw::FileWatcher>();
}

Matin::~Matin() {}

absl::Status Matin::Init(const Config& config) {
  settings_.directory = config.Get<std::string>("matin.directory");
  settings_.midi_grpc_host = config.Get<std::string>("matin.midi.grpc.host");
  settings_.midi_grpc_port = config.Get<int>("matin.midi.grpc.port");

  LOG(INFO) << "Matin initialized with settings: " << settings_.directory
            << ", " << settings_.midi_grpc_host << ", "
            << settings_.midi_grpc_port;

  file_watcher_->addWatch(settings_.directory,
                          static_cast<efsw::FileWatchListener*>(this), true);

  return absl::OkStatus();
}

absl::Status Matin::Run() {
  LOG(INFO) << "Matin running";

  file_watcher_->watch();

  return absl::OkStatus();
}

absl::Status Matin::Stop() {
  LOG(INFO) << "Matin stopping";

  file_watcher_->removeWatch(settings_.directory);
  file_watcher_.reset();

  return absl::OkStatus();
}

void Matin::handleFileAction(efsw::WatchID watchid, const std::string& dir,
                             const std::string& filename, efsw::Action action,
                             std::string oldFilename) {
  LOG(INFO) << "File action: " << filename;
}

}  // namespace maethstro
