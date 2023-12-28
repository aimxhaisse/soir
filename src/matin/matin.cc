#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <grpc++/grpc++.h>
#include <filesystem>
#include <regex>
#include <vector>

#include "common/utils.hh"
#include "matin.hh"

namespace maethstro {

Matin::Matin() : file_pattern_("^[a-z0-9_\\-]+\\.py") {
  file_watcher_ = std::make_unique<efsw::FileWatcher>();
}

Matin::~Matin() {}

absl::Status Matin::Init(const Config& config) {
  settings_.username = config.Get<int>("matin.username");
  settings_.directory = config.Get<std::string>("matin.directory");
  settings_.midi_grpc_host = config.Get<std::string>("matin.midi.grpc.host");
  settings_.midi_grpc_port = config.Get<int>("matin.midi.grpc.port");

  midi_stub_ = proto::Midi::NewStub(grpc::CreateChannel(
      settings_.midi_grpc_host + ":" + std::to_string(settings_.midi_grpc_port),
      grpc::InsecureChannelCredentials()));
  if (!midi_stub_) {
    return absl::InternalError("Failed to create MIDI gRPC stub");
  }

  file_watcher_->addWatch(settings_.directory,
                          static_cast<efsw::FileWatchListener*>(this), true);

  auto status = InitialLiveUpdate();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to send initial live update to Midi: "
               << status.message();
    return status;
  }

  LOG(INFO) << "Matin initialized with settings: " << settings_.directory
            << ", " << settings_.midi_grpc_host << ", "
            << settings_.midi_grpc_port;

  return absl::OkStatus();
}

absl::Status Matin::InitialLiveUpdate() const {
  std::vector<std::filesystem::path> files;
  std::copy(std::filesystem::directory_iterator(settings_.directory),
            std::filesystem::directory_iterator(), std::back_inserter(files));
  std::sort(files.begin(), files.end());
  for (const auto& file : files) {
    if (IsLiveCodingFile(file.filename())) {
      auto status_or = LiveUpdate(
          absl::StrCat(settings_.directory, "/", std::string(file.filename())));
      if (!status_or.ok()) {
        LOG(ERROR) << "Failed to send initial live update to Midi: "
                   << status_or.message();
        return status_or;
      }
    }
  }

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

bool Matin::IsLiveCodingFile(const std::string& filename) const {
  return std::regex_match(filename, file_pattern_);
}

absl::Status Matin::LiveUpdate(const std::string& filename) const {
  auto contents_or = utils::GetFileContents(filename);
  if (!contents_or.ok()) {
    LOG(ERROR) << "Failed to read file: " << contents_or.status().message();
    return contents_or.status();
  }

  grpc::ClientContext context;
  proto::MatinLiveUpdate_Request update;
  proto::MatinLiveUpdate_Response response;

  update.set_username(settings_.username);
  update.set_code(contents_or.value());

  grpc::Status status = midi_stub_->LiveUpdate(&context, update, &response);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to push Live update to Midi: "
               << status.error_message();
    return absl::InternalError("Failed to push Live update to Midi");
  }

  LOG(INFO) << "Live update pushed to Midi for file: " << filename;

  return absl::OkStatus();
}

void Matin::handleFileAction(efsw::WatchID watchid, const std::string& dir,
                             const std::string& filename, efsw::Action action,
                             std::string old_filename) {
  if (!(action == efsw::Actions::Modified || action == efsw::Actions::Add)) {
    return;
  }

  if (!IsLiveCodingFile(filename)) {
    return;
  }

  // Ignore errors here, we might be able to reconnect at a later stage.
  LiveUpdate(absl::StrCat(dir, filename)).IgnoreError();
}

}  // namespace maethstro
