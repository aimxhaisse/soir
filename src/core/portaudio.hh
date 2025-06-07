#pragma once

#include <absl/status/status.h>
#include <portaudio.h>
#include <string>
#include <vector>

namespace soir {
namespace portaudio {

struct Device {
  int id;
  std::string name;
};

void ListAudioOutDevices();
void ListAudioInDevices();

absl::Status GetAudioOutDevices(std::vector<Device>* out);
absl::Status GetAudioInDevices(std::vector<Device>* out);

absl::Status Initialize();
absl::Status Terminate();

}  // namespace portaudio
}  // namespace soir