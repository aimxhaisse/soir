#include <absl/log/log.h>

#include "portaudio.hh"

namespace soir {
namespace portaudio {

absl::Status Initialize() {
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    return absl::InternalError(
        std::string("Failed to initialize PortAudio: ") + Pa_GetErrorText(err));
  }
  return absl::OkStatus();
}

absl::Status Terminate() {
  PaError err = Pa_Terminate();
  if (err != paNoError) {
    return absl::InternalError(
        std::string("Failed to terminate PortAudio: ") + Pa_GetErrorText(err));
  }
  return absl::OkStatus();
}

absl::Status GetAudioOutDevices(std::vector<Device>* out) {
  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0) {
    return absl::InternalError(
        std::string("Failed to get device count: ") + Pa_GetErrorText(numDevices));
  }

  for (int i = 0; i < numDevices; i++) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
    if (deviceInfo && deviceInfo->maxOutputChannels > 0) {
      out->push_back({i, deviceInfo->name});
    }
  }
  return absl::OkStatus();
}

absl::Status GetAudioInDevices(std::vector<Device>* out) {
  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0) {
    return absl::InternalError(
        std::string("Failed to get device count: ") + Pa_GetErrorText(numDevices));
  }

  for (int i = 0; i < numDevices; i++) {
    const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo(i);
    if (deviceInfo && deviceInfo->maxInputChannels > 0) {
      out->push_back({i, deviceInfo->name});
    }
  }
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

}  // namespace portaudio
}  // namespace soir