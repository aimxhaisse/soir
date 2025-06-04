#include <absl/log/log.h>

#include "sdl.hh"

namespace soir {
namespace sdl {

absl::Status GetAudioOutDevices(std::vector<Device>* out) {
  int num = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num);
  for (int i = 0; i < num; i++) {
    out->push_back({i, SDL_GetAudioDeviceName(devices[i])});
  }
  SDL_free(devices);
  return absl::OkStatus();
}

absl::Status GetAudioInDevices(std::vector<Device>* out) {
  int num = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&num);
  for (int i = 0; i < num; i++) {
    out->push_back({i, SDL_GetAudioDeviceName(devices[i])});
  }
  SDL_free(devices);
  return absl::OkStatus();
}

void ListAudioOutDevices() {
  std::vector<Device> devices;
  auto status = GetAudioOutDevices(&devices);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get audio devices: " << status;
    return;
  }
  
  for (const auto& device : devices) {
    LOG(INFO) << "Audio device [" << device.id << "]: " << device.name;
  }
}

void ListAudioInDevices() {
  std::vector<Device> devices;
  auto status = GetAudioInDevices(&devices);
  if (!status.ok()) {
    LOG(ERROR) << "Failed to get audio input devices: " << status;
    return;
  }
  
  for (const auto& device : devices) {
    LOG(INFO) << "Audio input device [" << device.id << "]: " << device.name;
  }
}

}  // namespace sdl
}  // namespace soir