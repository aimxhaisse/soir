#pragma once

#include <thread>
#include <vector>

#include <absl/status/status.h>

#include "soir.grpc.pb.h"
#include "utils/config.hh"

namespace soir {
namespace core {
namespace test {

class NotificationRecorder {
 public:
  NotificationRecorder();
  ~NotificationRecorder();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status Run();

  std::vector<std::string> PopNotifications();

 private:
  std::thread thread_;
  std::unique_ptr<proto::Soir::Stub> soir_stub_;
  grpc::ClientContext context_;
  std::mutex mutex_;
  std::vector<std::string> notifications_;
};

}  // namespace test
}  // namespace core
}  // namespace soir
