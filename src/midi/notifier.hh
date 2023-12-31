#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>

#include "common/config.hh"
#include "live.grpc.pb.h"

namespace maethstro {
namespace midi {

// Wrapper around a gRPC writer to allow for safe notification.
class Subscriber {
 public:
  explicit Subscriber(
      grpc::ServerWriter<proto::MidiNotifications_Response>& writer);

  absl::Status Wait();
  absl::Status Notify(const proto::MidiNotifications_Response& notification);
  absl::Status AsyncStop();

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  bool stopped_ = false;
  grpc::ServerWriter<proto::MidiNotifications_Response>& writer_;
};

// Class to subscribe to notifications.
class Notifier {
 public:
  Notifier();
  ~Notifier();

  absl::Status Init(const common::Config& config);
  absl::Status Start();
  absl::Status Stop();

  // Ownership of the subscriber is handled by the caller, it must be
  // sure the subscriber pointer is always valid while subscribed.
  absl::Status Register(Subscriber* subscriber);
  absl::Status Unregister(Subscriber* subscriber);

  absl::Status Notify(const proto::MidiNotifications_Response& notification);

 private:
  absl::Status Run();

  std::thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool stopped_ = false;

  // Lifecycle is a bit tricky here:
  //
  // - subscribers are added to the list when they register
  // - subscribers are removed from the list when they disconnect
  // - or when the notifier is stopped
  //
  // Ownership is not managed here, it is handled by the called that
  // registers entries here. Once Stop() is called
  std::list<Subscriber*> subscribers_;
  std::list<proto::MidiNotifications_Response> notifications_;
};

}  // namespace midi
}  // namespace maethstro
