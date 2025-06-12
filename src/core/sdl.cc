#include <absl/log/log.h>

#include "sdl.hh"

namespace soir {
namespace sdl {

absl::Status Initialize() {
  if (!SDL_Init(SDL_INIT_AUDIO)) {
    return absl::InternalError(
        std::string("Failed to initialize SDL: ") + SDL_GetError());
  }
  return absl::OkStatus();
}

absl::Status Terminate() {
  SDL_Quit();
  return absl::OkStatus();
}

absl::Status GetAudioOutDevices(std::vector<Device>* out) {
  int num = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num);
  if (!devices) {
    return absl::InternalError(
        std::string("Failed to get playback devices: ") + SDL_GetError());
  }
  
  for (int i = 0; i < num; i++) {
    const char* name = SDL_GetAudioDeviceName(devices[i]);
    if (name) {
      out->push_back({i, name});
    }
  }
  SDL_free(devices);
  return absl::OkStatus();
}

absl::Status GetAudioInDevices(std::vector<Device>* out) {
  int num = 0;
  SDL_AudioDeviceID* devices = SDL_GetAudioRecordingDevices(&num);
  if (!devices) {
    return absl::InternalError(
        std::string("Failed to get recording devices: ") + SDL_GetError());
  }
  
  for (int i = 0; i < num; i++) {
    const char* name = SDL_GetAudioDeviceName(devices[i]);
    if (name) {
      out->push_back({i, name});
    }
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