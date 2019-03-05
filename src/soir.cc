#include <glog/logging.h>

#include "soir.h"

namespace soir {

// Relative path to the core configuration file.
constexpr const char *kCoreConfigPath = "etc/soir.yml";

Status Soir::Init() {
  StatusOr<std::unique_ptr<Config>> config_or =
      Config::LoadFromPath(kCoreConfigPath);
  if (!config_or.Ok()) {
    return config_or.GetStatus();
  }
  core_config_ = std::move(config_or.ValueOrDie());

  return StatusCode::OK;
}

Status Soir::Run() { return StatusCode::OK; }

} // namespace soir

using namespace soir;

int main(int ac, char **av) {
  FLAGS_logtostderr = 1;
  ::google::InitGoogleLogging(av[0]);

  Soir soir;

  Status status = soir.Init();
  if (status == StatusCode::OK) {
    status = soir.Run();
    LOG(INFO) << "Soir exited: " << status;
  } else {
    LOG(WARNING) << "Unable to init Soir: " << status;
  }

  return 0;
}
