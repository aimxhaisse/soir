#include <JuceHeader.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>
#include <memory>
#include <string>

#include "Maethstro.hh"

DEFINE_string(config, "etc/config.yaml",
              "path to the config file ('etc/config.yaml')");

int main(int argc, char* argv[]) {
  FLAGS_logtostderr = 1;
  ::google::InitGoogleLogging(argv[0]);
  ::gflags::ParseCommandLineFlags(&argc, &argv, true);

  maethstro::Maethstro maethstro;
  maethstro::Status status = maethstro.Init(FLAGS_config);
  if (status != maethstro::StatusCode::OK) {
    LOG(ERROR) << "Maethstro init failed with status=" << status;
    return 1;
  }

  status = maethstro.Run();
  if (status != maethstro::StatusCode::OK) {
    LOG(ERROR) << "Maethstro crashed with status=" << status;
    return 1;
  }

  LOG(ERROR) << "Maethstro exited with status=" << status;

  return 0;
}
