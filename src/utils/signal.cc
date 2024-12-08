#include <absl/log/log.h>
#include <condition_variable>
#include <csignal>
#include <ctime>
#include <mutex>

#include "signal.hh"

namespace soir {
namespace utils {

namespace {

std::mutex gExitMutex;
std::condition_variable gDoExit;
std::time_t gSignaledAt;
bool gKilled = false;

}  // namespace

constexpr int kSignalExpireDelaySec = 5;

namespace {

void HandleSignal(int sig) {
  if (sig == SIGINT || sig == SIGKILL) {
    std::unique_lock lock(gExitMutex);
    gSignaledAt = std::time(nullptr);
    if (sig == SIGKILL) {
      gKilled = true;
    }
  }
  gDoExit.notify_one();
}

}  // anonymous namespace

void SignalExit() {
  std::unique_lock lock(gExitMutex);
  gSignaledAt = std::time(nullptr);
  gKilled = true;
  gDoExit.notify_one();
}

absl::Status WaitForExitSignal() {
  std::signal(SIGINT, HandleSignal);

  LOG(INFO) << "Waiting for signal, press ctrl+c to exit...";

  std::time_t previous_signal_at = 0;
  std::unique_lock lock(gExitMutex);
  while (true) {
    if (gKilled) {
      LOG(INFO) << "Killed, exiting...";
      break;
    }

    if (gSignaledAt) {
      if ((gSignaledAt - previous_signal_at) < kSignalExpireDelaySec) {
        LOG(INFO) << "Interrupted twice in a short time, exiting...";
        break;
      } else {
        LOG(INFO)
            << "Are you sure to exit? ctrl+c to confirm (offer expires in "
            << kSignalExpireDelaySec << "seconds)";
      }
      previous_signal_at = gSignaledAt;
    }

    gDoExit.wait(lock);
  }

  return absl::OkStatus();
}

}  // namespace utils
}  // namespace soir
