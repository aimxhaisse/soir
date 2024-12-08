#include <absl/log/log.h>

#include "core/rt/notifier.hh"

namespace soir {
namespace rt {

Subscriber::Subscriber(grpc::ServerWriter<proto::GetLogsResponse>& writer)
    : writer_(writer) {}

absl::Status Subscriber::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  cond_.wait(lock, [this] { return stopped_; });

  return absl::OkStatus();
}

absl::Status Subscriber::AsyncStop() {
  std::lock_guard<std::mutex> lock(mutex_);
  stopped_ = true;
  cond_.notify_all();

  return absl::OkStatus();
}

absl::Status Subscriber::Notify(const proto::GetLogsResponse& response) {
  if (!writer_.Write(response)) {
    auto status = AsyncStop();
    if (!status.ok()) {
      LOG(WARNING) << "Failed to stop subscriber: " << status;
    }
  }

  return absl::OkStatus();
}

Notifier::Notifier() {}

Notifier::~Notifier() {}

absl::Status Notifier::Init(const utils::Config& config) {
  return absl::OkStatus();
}

absl::Status Notifier::Start() {
  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "Notifier failed: " << status;
    }
  });

  return absl::OkStatus();
}

absl::Status Notifier::Stop() {
  LOG(INFO) << "Stopping notifier";

  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto subscriber : subscribers_) {
      auto status = subscriber->AsyncStop();
      if (!status.ok()) {
        LOG(WARNING) << "Failed to stop subscriber: " << status;
      }
    }
    stopped_ = true;
    cond_.notify_all();
  }

  LOG(INFO) << "Waiting for notifier to stop";

  thread_.join();

  LOG(INFO) << "Notifier stopped";

  // We don't clear the list here, it is the caller's responsibility
  // to unregister subscribers.

  return absl::OkStatus();
}

absl::Status Notifier::Run() {
  while (true) {
    std::list<proto::GetLogsResponse> notifications;

    LOG(INFO) << "Waiting for notifications";

    {
      std::unique_lock<std::mutex> lock(mutex_);

      cond_.wait(lock, [this] { return stopped_ || !notifications_.empty(); });

      if (stopped_) {
        LOG(INFO)
            << "Done waiting for notifications as the notifier is stopping";
        break;
      }

      notifications.swap(notifications_);
    }

    LOG(INFO) << "Notifying subscribers";

    for (auto subscriber : subscribers_) {
      for (auto& notification : notifications) {
        auto status = subscriber->Notify(notification);
        if (!status.ok()) {
          LOG(WARNING) << "Failed to notify subscriber: " << status;
        }
      }
    }
  }

  return absl::OkStatus();
}

absl::Status Notifier::Register(Subscriber* subscriber) {
  std::lock_guard<std::mutex> lock(mutex_);

  subscribers_.push_back(subscriber);

  return absl::OkStatus();
}

absl::Status Notifier::Unregister(Subscriber* subscriber) {
  std::lock_guard<std::mutex> lock(mutex_);

  subscribers_.remove(subscriber);

  return absl::OkStatus();
}

absl::Status Notifier::Notify(const proto::GetLogsResponse& notification) {
  std::lock_guard<std::mutex> lock(mutex_);

  notifications_.push_back(notification);

  cond_.notify_all();

  return absl::OkStatus();
}

}  // namespace rt
}  // namespace soir
