#include "live.hh"

#include "matin/matin.hh"

int main(int ac, char** av) {
  LOG(INFO) << "live version: " << std::string(live::kVersion);

  maethstro::Matin matin;

  absl::Status status = matin.Run();

  return status.raw_code();
}
