#include <glog/logging.h>

#include "soir.h"

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
