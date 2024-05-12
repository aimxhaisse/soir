#include <absl/log/log.h>
#include <absl/strings/str_cat.h>
#include <grpc++/grpc++.h>
#include <filesystem>

#include "agent/file_watcher.hh"
#include "utils/config.hh"
#include "utils/misc.hh"

namespace neon {
namespace agent {

FileWatcher::FileWatcher() : file_pattern_("^[a-z0-9_\\-]+\\.py") {
  file_watcher_ = std::make_unique<efsw::FileWatcher>();
}

FileWatcher::~FileWatcher() {}

absl::Status FileWatcher::Init(const utils::Config& config,
                               proto::Neon::Stub* stub) {
  neon_stub_ = stub;

  directory_ = config.Get<std::string>("agent.directory");

  file_watcher_ = std::make_unique<efsw::FileWatcher>();

  file_watcher_->addWatch(directory_,
                          static_cast<efsw::FileWatchListener*>(this), true);

  auto status = InitialCodeUpdate();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to send initial live update to Neon: "
               << status.message();
    return status;
  }

  LOG(INFO) << "File watcher initialized";

  return absl::OkStatus();
}

absl::Status FileWatcher::Start() {
  thread_ = std::thread([this]() {
    file_watcher_->watch();
    LOG(INFO) << "File watcher started";
  });

  return absl::OkStatus();
}

absl::Status FileWatcher::Stop() {
  file_watcher_->removeWatch(directory_);
  file_watcher_.reset();

  if (thread_.joinable()) {
    thread_.join();
  }

  neon_stub_ = nullptr;

  LOG(INFO) << "File watcher stopped";

  return absl::OkStatus();
}

absl::Status FileWatcher::SendCodeUpdate(const std::string& filename) const {
  auto contents_or = utils::GetFileContents(filename);
  if (!contents_or.ok()) {
    LOG(ERROR) << "Failed to read file: " << contents_or.status().message();
    return contents_or.status();
  }

  grpc::ClientContext context;
  proto::PushCodeUpdateRequest update;
  proto::PushCodeUpdateResponse response;

  update.set_code(contents_or.value());

  grpc::Status status = neon_stub_->PushCodeUpdate(&context, update, &response);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to push code update to Neon: "
               << status.error_message();
    return absl::InternalError("Failed to push code update to Neon");
  }

  LOG(INFO) << "Code update pushed to Neon for file: " << filename;

  return absl::OkStatus();
}

absl::Status FileWatcher::InitialCodeUpdate() const {
  std::vector<std::filesystem::path> files;
  std::copy(std::filesystem::directory_iterator(directory_),
            std::filesystem::directory_iterator(), std::back_inserter(files));
  std::sort(files.begin(), files.end());
  for (const auto& file : files) {
    if (IsLiveCodingFile(file.filename())) {
      auto status_or = SendCodeUpdate(
          absl::StrCat(directory_, "/", std::string(file.filename())));
      if (!status_or.ok()) {
        LOG(ERROR) << "Failed to send initial live update to Neon: "
                   << status_or.message();
        return status_or;
      }
    }
  }

  return absl::OkStatus();
}

void FileWatcher::handleFileAction(efsw::WatchID watchid,
                                   const std::string& dir,
                                   const std::string& filename,
                                   efsw::Action action,
                                   std::string old_filename) {
  if (!(action == efsw::Actions::Modified || action == efsw::Actions::Add)) {
    return;
  }

  if (!IsLiveCodingFile(filename)) {
    return;
  }

  // Ignore errors here, we might be able to reconnect at a later stage.
  SendCodeUpdate(absl::StrCat(dir, filename)).IgnoreError();
}

bool FileWatcher::IsLiveCodingFile(const std::string& filename) const {
  return std::regex_match(filename, file_pattern_);
}

}  // namespace agent
}  // namespace neon
