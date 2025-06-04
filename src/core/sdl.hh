#pragma once

#include <SDL3/SDL.h>
#include <absl/status/status.h>
#include <string>
#include <vector>

namespace soir {
namespace sdl {

struct Device {
  int id;
  std::string name;
};

void ListAudioOutDevices();
void ListAudioInDevices();

absl::Status GetAudioOutDevices(std::vector<Device>* out);
absl::Status GetAudioInDevices(std::vector<Device>* out);

}  // namespace sdl
}  // namespace soir