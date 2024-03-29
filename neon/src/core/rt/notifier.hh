#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <list>

#include "neon.grpc.pb.h"
#include "utils/config.hh"

namespace neon {
namespace rt {

// Wrapper around a gRPC writer to allow for safe notification.
class Subscriber {
 public:
  explicit Subscriber(grpc::ServerWriter<proto::GetLogsResponse>& writer);

  absl::Status Wait();
  absl::Status Notify(const proto::GetLogsResponse& notification);
  absl::Status AsyncStop();

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  bool stopped_ = false;
  grpc::ServerWriter<proto::GetLogsResponse>& writer_;
};

// Class to subscribe to notifications.
class Notifier {
 public:
  Notifier();
  ~Notifier();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  // Ownership of the subscriber is handled by the caller, it must be
  // sure the subscriber pointer is always valid while subscribed.
  absl::Status Register(Subscriber* subscriber);
  absl::Status Unregister(Subscriber* subscriber);

  absl::Status Notify(const proto::GetLogsResponse& notification);

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
  std::list<proto::GetLogsResponse> notifications_;
};

}  // namespace rt
}  // namespace neon
