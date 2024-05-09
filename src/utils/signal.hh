#pragma once

#include <absl/status/status.h>

namespace neon {
namespace utils {

// Wait for the server to be killed by a signal. This is in a
// dedicated file as this code tends to not be portable, better
// isolate it to get rid of the tricky implementation details.
absl::Status WaitForExitSignal();

void SignalExit();

}  // namespace utils
}  // namespace neon
