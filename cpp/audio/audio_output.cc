#define MINIAUDIO_IMPLEMENTATION
#include "audio/audio_output.hh"

#include <algorithm>

#include "absl/log/log.h"
#include "audio/audio_buffer.hh"

namespace soir {
namespace audio {

namespace {

// Device-side buffer headroom (in periods) for scheduling jitter; costs
// monitoring latency (~43 ms at 48 kHz). A hint only on some backends.
static constexpr int kDeviceBufferPeriods = 4;

// Silence slots ahead of the first block: headroom for engine stalls on
// top of the device buffer.
static constexpr int kPrefillSlots = 2;

void PopulateDevice(int id, const ma_device_info& info, Device* dev) {
  dev->id = id;
  dev->name = info.name;
  dev->is_default = info.isDefault != 0;
  dev->channels = 0;

  for (ma_uint32 j = 0; j < info.nativeDataFormatCount; j++) {
    const auto& fmt = info.nativeDataFormats[j];
    int channels = (fmt.channels == 0) ? 64 : static_cast<int>(fmt.channels);
    if (channels > dev->channels) {
      dev->channels = channels;
    }
  }
}

}  // namespace

absl::StatusOr<std::vector<Device>> GetAudioOutDevices() {
  ma_context context;
  if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
    return absl::InternalError("Failed to initialize miniaudio context");
  }

  ma_device_info* playback_infos;
  ma_uint32 playback_count;
  ma_device_info* capture_infos;
  ma_uint32 capture_count;

  if (ma_context_get_devices(&context, &playback_infos, &playback_count,
                             &capture_infos, &capture_count) != MA_SUCCESS) {
    ma_context_uninit(&context);
    return absl::InternalError("Failed to enumerate audio devices");
  }

  std::vector<Device> devices;
  for (ma_uint32 i = 0; i < playback_count; i++) {
    ma_device_info detailed_info;
    if (ma_context_get_device_info(&context, ma_device_type_playback,
                                   &playback_infos[i].id,
                                   &detailed_info) == MA_SUCCESS) {
      Device dev;
      PopulateDevice(i, detailed_info, &dev);
      devices.push_back(dev);
    }
  }

  ma_context_uninit(&context);
  return devices;
}

absl::StatusOr<std::vector<Device>> GetAudioInDevices() {
  ma_context context;
  if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
    return absl::InternalError("Failed to initialize miniaudio context");
  }

  ma_device_info* playback_infos;
  ma_uint32 playback_count;
  ma_device_info* capture_infos;
  ma_uint32 capture_count;

  if (ma_context_get_devices(&context, &playback_infos, &playback_count,
                             &capture_infos, &capture_count) != MA_SUCCESS) {
    ma_context_uninit(&context);
    return absl::InternalError("Failed to enumerate audio devices");
  }

  std::vector<Device> devices;
  for (ma_uint32 i = 0; i < capture_count; i++) {
    ma_device_info detailed_info;
    if (ma_context_get_device_info(&context, ma_device_type_capture,
                                   &capture_infos[i].id,
                                   &detailed_info) == MA_SUCCESS) {
      Device dev;
      PopulateDevice(i, detailed_info, &dev);
      devices.push_back(dev);
    }
  }

  ma_context_uninit(&context);
  return devices;
}

static void data_callback(ma_device* device, void* output, const void* input,
                          ma_uint32 frame_count) {
  (void)input;

  auto* audio_output = static_cast<AudioOutput*>(device->pUserData);
  float* output_buffer = static_cast<float*>(output);
  ma_uint32 samples_needed = frame_count * device->playback.channels;

  if (audio_output == nullptr) {
    memset(output_buffer, 0, samples_needed * sizeof(float));
    return;
  }

  audio_output->FillCallbackBuffer(output_buffer, samples_needed);
}

void AudioOutput::FillCallbackBuffer(float* output, size_t samples_needed) {
  uint64_t read_seq = read_seq_local_;

  // The engine may have dropped stale slots: skip to the oldest it still
  // holds, on a slot boundary.
  const uint64_t floor = drop_seq_.load(std::memory_order_acquire);
  if (read_seq < floor) {
    read_seq = floor;
    read_offset_ = 0;
  }

  const uint64_t write_seq = write_seq_.load(std::memory_order_acquire);
  size_t copied = 0;
  uint64_t seq = read_seq;
  while (copied < samples_needed && seq != write_seq) {
    const float* src =
        slots_[seq % kOutputSlotCount].samples + read_offset_ * kNumChannels;
    const size_t available =
        static_cast<size_t>(kOutputSlotFrames - read_offset_) * kNumChannels;
    const size_t take = std::min(available, samples_needed - copied);

    memcpy(output + copied, src, take * sizeof(float));
    copied += take;

    read_offset_ += static_cast<int>(take / kNumChannels);
    if (read_offset_ == kOutputSlotFrames) {
      read_offset_ = 0;
      seq++;
    }
  }

  read_seq_local_ = seq;
  read_seq_.store(seq, std::memory_order_release);

  if (copied < samples_needed) {
    memset(output + copied, 0, (samples_needed - copied) * sizeof(float));
    if (prefilled_.load(std::memory_order_relaxed)) {
      OnUnderrun(samples_needed - copied);
    }
  }
}

void AudioOutput::OnUnderrun(size_t missing_samples) {
  const uint64_t total = underruns_.fetch_add(1, std::memory_order_relaxed) + 1;
  const absl::Time now = absl::Now();
  if (now - last_underrun_warn_ >= absl::Seconds(1)) {
    last_underrun_warn_ = now;
    LOG(WARNING) << "Audio output underrun: inserted " << missing_samples
                 << " samples of silence (" << total
                 << " underrun callbacks so far). The engine is not "
                    "pushing audio fast enough; check the log for "
                    "long FastUpdate durations or stalls";
  }
}

void AudioOutput::WriteSlot(const float* left, const float* right,
                            size_t frames) {
  const uint64_t write_seq = write_seq_local_;

  // The device thread may not have published its progress since our last
  // trim, so never consider it below the floor we already gave it.
  uint64_t read_seq = read_seq_.load(std::memory_order_acquire);
  if (read_seq < drop_seq_local_) {
    read_seq = drop_seq_local_;
  }

  // Cap the backlog: a fast-forward burst after a stall can push seconds
  // at once, so drop the oldest slots past the cap and let the device
  // thread skip forward. Never triggers in steady state.
  if (write_seq - read_seq >= kOutputMaxBacklogSlots) {
    drop_seq_local_ = write_seq - kOutputMaxBacklogSlots + 1;
    drop_seq_.store(drop_seq_local_, std::memory_order_release);
  }

  float* dst = slots_[write_seq % kOutputSlotCount].samples;
  size_t i = 0;
  for (; i < frames; i++) {
    dst[i * kNumChannels] = left[i];
    dst[i * kNumChannels + 1] = right[i];
  }
  // A short tail must not leak the previous occupant of the slot.
  for (; i < static_cast<size_t>(kOutputSlotFrames); i++) {
    dst[i * kNumChannels] = 0.0f;
    dst[i * kNumChannels + 1] = 0.0f;
  }

  write_seq_local_ = write_seq + 1;
  write_seq_.store(write_seq_local_, std::memory_order_release);
}

AudioOutput::AudioOutput()
    : context_initialized_(false),
      device_(new ma_device()),
      initialized_(false),
      underruns_(0),
      last_underrun_warn_{},
      write_seq_(0),
      read_seq_(0),
      drop_seq_(0),
      write_seq_local_(0),
      drop_seq_local_(0),
      read_seq_local_(0),
      read_offset_(0),
      prefilled_(false) {}

AudioOutput::~AudioOutput() {
  if (initialized_) {
    ma_device_uninit(device_);
  }
  delete device_;
  if (context_initialized_) {
    ma_context_uninit(&context_);
  }
}

void AudioOutput::Reset() {
  write_seq_local_ = 0;
  drop_seq_local_ = 0;
  read_seq_local_ = 0;
  read_offset_ = 0;

  write_seq_.store(0, std::memory_order_release);
  read_seq_.store(0, std::memory_order_release);
  drop_seq_.store(0, std::memory_order_release);

  prefilled_.store(false, std::memory_order_relaxed);
  underruns_.store(0, std::memory_order_relaxed);
  last_underrun_warn_ = absl::Time();
}

absl::Status AudioOutput::Init(int sample_rate, int channels, int buffer_size,
                               const std::string& device_name) {
  if (channels != kNumChannels) {
    return absl::InvalidArgumentError(
        "Audio output requires exactly 2 channels");
  }

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_f32;
  config.playback.channels = channels;
  config.sampleRate = sample_rate;
  config.dataCallback = data_callback;
  config.pUserData = this;
  config.periodSizeInFrames = buffer_size;
  config.periods = kDeviceBufferPeriods;

  if (!device_name.empty()) {
    if (ma_context_init(nullptr, 0, nullptr, &context_) == MA_SUCCESS) {
      context_initialized_ = true;

      ma_device_info* playback_infos;
      ma_uint32 playback_count;
      ma_device_info* capture_infos;
      ma_uint32 capture_count;
      bool found = false;

      if (ma_context_get_devices(&context_, &playback_infos, &playback_count,
                                 &capture_infos,
                                 &capture_count) == MA_SUCCESS) {
        for (ma_uint32 i = 0; i < playback_count; i++) {
          if (std::string(playback_infos[i].name).find(device_name) !=
              std::string::npos) {
            selected_device_id_ = playback_infos[i].id;
            config.playback.pDeviceID = &selected_device_id_;
            found = true;
            LOG(INFO) << "Selected audio output device: "
                      << playback_infos[i].name;
            break;
          }
        }
      }

      if (!found) {
        LOG(WARNING) << "Audio output device not found: " << device_name
                     << ", falling back to system default";
      }
    } else {
      LOG(WARNING) << "Failed to enumerate audio devices, using system default";
    }
  }

  ma_context* pContext = context_initialized_ ? &context_ : nullptr;
  if (ma_device_init(pContext, &config, device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to initialize audio device");
  }

  initialized_ = true;
  LOG(INFO) << "Audio output initialized: " << sample_rate << "Hz, " << channels
            << " channels, " << buffer_size << " frames";

  return absl::OkStatus();
}

absl::Status AudioOutput::Start() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Audio output not initialized");
  }

  // Device callback not running yet: clear the ring so audio queued
  // before the last Stop() does not play at startup.
  Reset();

  if (ma_device_start(device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to start audio device");
  }

  LOG(INFO) << "Audio output started";
  return absl::OkStatus();
}

absl::Status AudioOutput::Stop() {
  if (!initialized_) {
    return absl::FailedPreconditionError("Audio output not initialized");
  }

  if (ma_device_stop(device_) != MA_SUCCESS) {
    return absl::InternalError("Failed to stop audio device");
  }

  LOG(INFO) << "Audio output stopped";
  return absl::OkStatus();
}

absl::Status AudioOutput::PushAudioBuffer(AudioBuffer& buffer) {
  const size_t frames = buffer.Size();
  if (frames == 0) {
    return absl::OkStatus();
  }

  const float* left = buffer.GetChannel(kLeftChannel);
  const float* right = buffer.GetChannel(kRightChannel);

  if (!prefilled_.exchange(true)) {
    for (int i = 0; i < kPrefillSlots; i++) {
      WriteSlot(nullptr, nullptr, 0);
    }
  }

  for (size_t offset = 0; offset < frames; offset += kOutputSlotFrames) {
    const size_t chunk =
        std::min(static_cast<size_t>(kOutputSlotFrames), frames - offset);
    WriteSlot(left + offset, right + offset, chunk);
  }

  return absl::OkStatus();
}

}  // namespace audio
}  // namespace soir
