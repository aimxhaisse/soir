#include "live.hh"

int main(int ac, char** av) {
  LOG(INFO) << "live version: " << std::string(live::kVersion);

  absl::Status status = absl::OkStatus();

  return status.raw_code();
}
