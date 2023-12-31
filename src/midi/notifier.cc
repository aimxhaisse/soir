#include <absl/log/log.h>

#include "notifier.hh"

namespace maethstro {
namespace midi {

Subscriber::Subscriber(
    grpc::ServerWriter<proto::MidiNotifications_Response>& writer)
    : writer_(writer) {}

void Subscriber::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);

  stopped_ = true;

  cond_.notify_all();
}

absl::Status Subscriber::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);

  if (stopped_) {
    return absl::OkStatus();
  }

  cond_.wait(lock, [this] { return stopped_; });

  return absl::OkStatus();
}

absl::Status Subscriber::Notify(
    const proto::MidiNotifications_Response& response) {
  if (!writer_.Write(response)) {
    Stop();
  }

  return absl::OkStatus();
}

Notifier::Notifier() {}

Notifier::~Notifier() {}

absl::Status Notifier::Init(const Config& config) {
  return absl::OkStatus();
}

absl::Status Notifier::Stop() {
  std::lock_guard<std::mutex> lock(mutex_);

  stopped_ = true;

  for (auto subscriber : subscribers_) {
    subscriber->Stop();
  }

  // We don't clear the list here, it is the caller's responsibility
  // to unregister subscribers.

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

absl::Status Notifier::Notify(
    const proto::MidiNotifications_Response& notification) {
  std::lock_guard<std::mutex> lock(mutex_);

  for (auto subscriber : subscribers_) {
    auto status = subscriber->Notify(notification);
    if (!status.ok()) {
      LOG(ERROR) << "Unable to notify subscriber: " << status;
    }
  }

  return absl::OkStatus();
}

}  // namespace midi
}  // namespace maethstro
