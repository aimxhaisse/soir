#include "inst/external.hh"

#include <absl/log/log.h>
#include <absl/status/status.h>

#include <nlohmann/json.hpp>

#include "core/common.hh"
#include "inst/midi_backend.hh"

namespace {

bool useMidiPort(const std::string midi_out, libremidi::midi_out& libremidi) {
  auto ports =
      libremidi::observer{
          {},
          libremidi::observer_configuration_for(libremidi.get_current_api())}
          .get_output_ports();

  for (auto& port : ports) {
    if (port.display_name != midi_out) {
      continue;
    }
    LOG(INFO) << "Found MIDI out port " << port.display_name;
    libremidi.open_port(port);
    if (!libremidi.is_port_open()) {
      LOG(ERROR) << "Failed to open MIDI out port " << port.display_name;
      return false;
    }
    return true;
  }

  LOG(ERROR) << "MIDI out port " << midi_out << " not found";

  return false;
}

}  // namespace

namespace soir {
namespace inst {

void External::AudioInputCallback(ma_device* device, void* output,
                                  const void* input, ma_uint32 frame_count) {
  External* ext = static_cast<External*>(device->pUserData);
  if (!input || !ext) {
    return;
  }

  ext->NoteCallbackSize(frame_count);

  float* write_ptr;
  const ma_uint32 requested = frame_count;
  ma_pcm_rb_acquire_write(&ext->audio_ringbuffer_, &frame_count,
                          (void**)&write_ptr);
  if (frame_count > 0) {
    memcpy(write_ptr, input,
           frame_count * ext->audio_in_chans_ * sizeof(float));
    ma_pcm_rb_commit_write(&ext->audio_ringbuffer_, frame_count);
  }

  if (frame_count < requested) {
    // The ring buffer was not empty enough to take the whole callback:
    // the missing frames are dropped input.
    ext->CountDroppedFrames(requested - frame_count);
  }
}

void External::NoteCallbackSize(ma_uint32 frame_count) {
  // Log the negotiated period once (from the first callback, before
  // acquire_write can reduce frame_count): it determines how much
  // input a single callback delivers, which the ring buffer must be
  // able to absorb.
  if (!callback_logged_.exchange(true)) {
    LOG(INFO) << "Audio input callback delivers " << frame_count
              << " frames per period (" << (frame_count * 1000) / kSampleRate
              << " ms)";
  }
}

void External::CountDroppedFrames(uint64_t frames) {
  const uint64_t total =
      dropped_frames_.fetch_add(frames, std::memory_order_relaxed) + frames;
  const absl::Time now = absl::Now();
  // Rate-limited: at most one warning every 2 seconds, and only while
  // input is actually being lost.
  if (now - last_drop_warn_ >= absl::Seconds(2)) {
    last_drop_warn_ = now;
    LOG(WARNING) << "Audio input ring buffer is full: dropped " << total
                 << " frames of input so far. The capture period is "
                    "larger than the input path can absorb; the "
                    "external audio will crackle until this stops";
  }
}

void External::ProcessAudioInput() {
  if (!audio_in_device_initialized_) {
    return;
  }

  if (ma_pcm_rb_available_read(&audio_ringbuffer_) < kBlockSize) {
    return;
  }

  int left_channel = channel_map_[0];
  int right_channel = channel_map_[1];

  // The capture device's period is negotiated with the driver and may
  // be several kBlockSize frames, so a single callback can deliver
  // several blocks at once: drain as many full blocks as are
  // available (bounded, so a burst of input can't stall the External
  // thread for long).
  std::vector<float> input_buffer(kBlockSize * audio_in_chans_);
  for (int drained = 0; drained < kMaxDrainBlocks; ++drained) {
    ma_uint32 available_frames = ma_pcm_rb_available_read(&audio_ringbuffer_);
    if (available_frames < kBlockSize) {
      break;
    }

    void* read_ptr;
    ma_uint32 frames_to_read = kBlockSize;
    ma_pcm_rb_acquire_read(&audio_ringbuffer_, &frames_to_read, &read_ptr);
    if (frames_to_read < kBlockSize) {
      ma_pcm_rb_commit_read(&audio_ringbuffer_, frames_to_read);
      break;
    }

    memcpy(input_buffer.data(), read_ptr,
           kBlockSize * audio_in_chans_ * sizeof(float));
    ma_pcm_rb_commit_read(&audio_ringbuffer_, kBlockSize);

    AudioBuffer output_buffer(kBlockSize);
    float* left = output_buffer.GetChannel(kLeftChannel);
    float* right = output_buffer.GetChannel(kRightChannel);

    for (int i = 0; i < kBlockSize; ++i) {
      left[i] = input_buffer[i * audio_in_chans_ + left_channel];
      right[i] = input_buffer[i * audio_in_chans_ + right_channel];
    }

    {
      std::lock_guard<std::mutex> lock(mutex_);
      buffers_.push_back(std::move(output_buffer));
      // Cap the backlog: if the consumer (Render) is not keeping up
      // (engine stalled), drop the oldest blocks instead of growing
      // unboundedly or adding unbounded latency.
      while (buffers_.size() > kMaxInputBacklog) {
        buffers_.pop_front();
      }
    }
  }
}

External::External() {
  audio_in_context_initialized_ = false;
  audio_in_device_initialized_ = false;
}

External::~External() {
  if (audio_in_device_initialized_) {
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
  }
  if (audio_in_context_initialized_) {
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
  }
}

absl::Status External::GetMidiDevices(
    std::vector<std::pair<int, std::string>>* out) {
  if (!MidiBackendAvailable()) {
    LOG(WARNING) << "MIDI backend unavailable, no MIDI out devices listed";
    return absl::OkStatus();
  }

  libremidi::midi_out midi_out;
  auto ports =
      libremidi::observer{
          {}, libremidi::observer_configuration_for(midi_out.get_current_api())}
          .get_output_ports();
  int i = 0;
  for (auto& port : ports) {
    out->emplace_back(i, port.display_name);
    i++;
  }
  return absl::OkStatus();
}

absl::Status External::ParseAndValidateSettings(
    const std::string& settings, std::optional<std::string>* midi_out_device,
    std::optional<std::string>* audio_in_device, std::vector<int>* channels) {
  auto params = nlohmann::json::parse(settings, nullptr, false);

  if (params.contains("midi_out") && params["midi_out"].is_string()) {
    *midi_out_device = params["midi_out"].get<std::string>();
  } else {
    *midi_out_device = std::nullopt;
  }

  if (params.contains("audio_in") && params["audio_in"].is_string()) {
    *audio_in_device = params["audio_in"].get<std::string>();
  } else {
    *audio_in_device = std::nullopt;
  }

  if (!midi_out_device->has_value() && !audio_in_device->has_value()) {
    return absl::InvalidArgumentError(
        "At least one of midi_out or audio_in must be specified");
  }

  if (audio_in_device->has_value()) {
    if (!params.contains("audio_channels") ||
        !params["audio_channels"].is_array()) {
      return absl::InvalidArgumentError(
          "audio_channels required when audio_in is set");
    }

    const auto& audio_chans = params["audio_channels"];
    if (audio_chans.size() != 2) {
      return absl::InvalidArgumentError(
          "audio_channels must have exactly 2 elements [L, R]");
    }

    channels->clear();
    for (const auto& chan : audio_chans) {
      if (!chan.is_number_integer()) {
        return absl::InvalidArgumentError("Audio channels must be integers");
      }
      channels->push_back(chan.get<int>());
    }
  }

  return absl::OkStatus();
}

absl::Status External::ConfigureMidiPort(
    const std::optional<std::string>& midi_out_device) {
  if (!midi_out_device.has_value()) {
    settings_midi_out_ = std::nullopt;
    return absl::OkStatus();
  }

  if (midi_out_device == settings_midi_out_ && midi_out_.has_value() &&
      midi_out_->is_port_open()) {
    return absl::OkStatus();
  }

  if (!MidiBackendAvailable()) {
    LOG(ERROR) << "MIDI backend unavailable, cannot open MIDI port "
               << *midi_out_device;
    return absl::OkStatus();
  }

  if (!midi_out_.has_value()) {
    midi_out_.emplace();
  }
  midi_out_->close_port();
  LOG(INFO) << "Trying to open MIDI port " << *midi_out_device << "...";

  if (!useMidiPort(*midi_out_device, *midi_out_)) {
    LOG(WARNING) << "Failed to open MIDI port " << *midi_out_device;
    return absl::OkStatus();
  }

  settings_midi_out_ = midi_out_device;
  return absl::OkStatus();
}

absl::Status External::ConfigureAudioDevice(
    const std::optional<std::string>& audio_in_device,
    const std::vector<int>& channels) {
  if (!audio_in_device.has_value()) {
    settings_audio_in_ = std::nullopt;
    return absl::OkStatus();
  }

  const bool chans_changed = (settings_chans_ != channels);
  const bool device_changed = (audio_in_device != settings_audio_in_);

  if (!device_changed && !chans_changed && audio_in_device_initialized_) {
    return absl::OkStatus();
  }

  if (audio_in_device_initialized_) {
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
  }
  if (audio_in_context_initialized_) {
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
  }

  LOG(INFO) << "Trying to open audio device " << *audio_in_device << "...";

  for (int channel : channels) {
    if (channel < 0) {
      LOG(WARNING) << "Invalid channel index: " << channel;
      return absl::OkStatus();
    }
  }

  int max_channel = *std::max_element(channels.begin(), channels.end());
  int required_channels = max_channel + 1;

  // The ring must absorb at least the largest period the driver might
  // negotiate for the capture device (we request kBlockSize, but the
  // driver is free to give more). 16 blocks (170 ms) covers periods
  // up to 8 blocks; ProcessAudioInput drains several blocks per call
  // to stay ahead of larger ones.
  ma_uint32 ringbuffer_size = kBlockSize * 16 * required_channels;
  ma_result result =
      ma_pcm_rb_init(ma_format_f32, required_channels, ringbuffer_size, nullptr,
                     nullptr, &audio_ringbuffer_);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to initialize ringbuffer: " << result;
    return absl::OkStatus();
  }

  ma_device_config config = ma_device_config_init(ma_device_type_capture);
  config.capture.format = ma_format_f32;
  config.capture.channels = required_channels;
  config.sampleRate = kSampleRate;
  // Request the same period as the engine's block size. Without this,
  // miniaudio's ALSA backend defaults to a 100 ms period, and a
  // single 4800-frame callback would overflow the input ring and drop
  // most of the external audio.
  config.periodSizeInFrames = kBlockSize;
  config.periods = 2;
  config.dataCallback = AudioInputCallback;
  config.pUserData = this;

  ma_device_info* capture_devices;
  ma_uint32 capture_device_count;

  result = ma_context_init(nullptr, 0, nullptr, &audio_in_context_);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to initialize audio context: " << result;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }
  audio_in_context_initialized_ = true;

  result = ma_context_get_devices(&audio_in_context_, nullptr, nullptr,
                                  &capture_devices, &capture_device_count);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to enumerate audio devices: " << result;
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  bool found = false;
  for (ma_uint32 i = 0; i < capture_device_count; ++i) {
    if (*audio_in_device == capture_devices[i].name) {
      config.capture.pDeviceID = &capture_devices[i].id;
      found = true;
      break;
    }
  }

  if (!found) {
    LOG(WARNING) << "Audio device not found: " << *audio_in_device;
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  result = ma_device_init(&audio_in_context_, &config, &audio_in_device_);

  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to open audio device: " << result;
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  result = ma_device_start(&audio_in_device_);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to start audio device: " << result;
    ma_device_uninit(&audio_in_device_);
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  audio_in_device_initialized_ = true;
  audio_in_chans_ = required_channels;
  channel_map_ = channels;
  settings_audio_in_ = audio_in_device;
  settings_chans_ = channels;

  LOG(INFO) << "Audio input device " << *audio_in_device << " configured with "
            << required_channels << " channels at " << kSampleRate << " Hz";

  return absl::OkStatus();
}

absl::Status External::Init(const std::string& settings,
                            SampleManager* sample_manager, Controls* controls) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings == settings_) {
    return absl::OkStatus();
  }

  std::optional<std::string> midi_out_device;
  std::optional<std::string> audio_in_device;
  std::vector<int> channels;

  auto status = ParseAndValidateSettings(settings, &midi_out_device,
                                         &audio_in_device, &channels);
  if (!status.ok()) {
    LOG(WARNING) << "Invalid settings: " << status.message();
    return absl::OkStatus();
  }

  status = ConfigureMidiPort(midi_out_device);
  if (!status.ok()) {
    return status;
  }

  status = ConfigureAudioDevice(audio_in_device, channels);
  if (!status.ok()) {
    return status;
  }

  settings_ = settings;
  return absl::OkStatus();
}

void External::ScheduleMidiEvents(
    std::chrono::steady_clock::time_point block_at) {
  uint32_t current_tick;
  MidiStack events;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<MidiEventAt> events_at;
    midi_stack_.EventsAtTick(current_tick_ + kBlockSize, events_at);
    events.AddEvents(events_at);
    current_tick = current_tick_;
  }

  const uint32_t nsamples = std::min(kMidiExtChunkSize, kBlockSize);
  const int64_t nus = (static_cast<int64_t>(nsamples) * 1000000) / kSampleRate;

  int chunk = 0;
  do {
    const auto chunk_at =
        block_at + std::chrono::microseconds(static_cast<int64_t>(chunk) * nus);
    std::this_thread::sleep_until(chunk_at);
    std::list<MidiEventAt> events_at;
    events.EventsAtTick(current_tick + (1 + chunk) * nsamples, events_at);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (settings_midi_out_.has_value() && midi_out_.has_value()) {
        for (auto& ev : events_at) {
          midi_out_->send_message(ev.Msg());
        }
      }
    }

    chunk += 1;
  } while ((chunk * nsamples) < kBlockSize);
}

absl::Status External::Start() {
  LOG(INFO) << "Starting External thread";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "External thread failed: " << status;
    }
  });

  return absl::OkStatus();
}

void External::WaitForInitialTick() {
  while (true) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (current_tick_) {
        break;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

absl::Status External::Run() {
  WaitForInitialTick();

  // Steady clock: the wall clock (absl::Now) can be stepped by NTP or
  // VM time synchronisation, which would stretch or skip the soft
  // clock and drop/duplicate blocks of MIDI events and input audio.
  const auto block_duration =
      std::chrono::microseconds((1000000LL * kBlockSize) / kSampleRate);
  auto next_block_at = std::chrono::steady_clock::now();
  const auto initial_time = next_block_at;
  uint32_t block_count = 0;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, next_block_at, [this]() { return stop_; });
      if (stop_) {
        break;
      }
    }

    ScheduleMidiEvents(next_block_at);
    ProcessAudioInput();

    block_count += 1;
    next_block_at = initial_time + (block_count * block_duration);
    current_tick_ += kBlockSize;
  }

  return absl::OkStatus();
}

absl::Status External::Stop() {
  LOG(INFO) << "Stopping External thread";

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cv_.notify_all();
  }

  if (thread_.joinable()) {
    thread_.join();
  }

  LOG(INFO) << "External thread stopped";

  if (audio_in_device_initialized_) {
    ma_device_stop(&audio_in_device_);
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
  }
  if (audio_in_context_initialized_) {
    ma_context_uninit(&audio_in_context_);
    audio_in_context_initialized_ = false;
  }

  if (midi_out_.has_value() && midi_out_->is_port_open()) {
    midi_out_->close_port();
  }

  return absl::OkStatus();
}

void External::Render(SampleTick tick, const std::list<MidiEventAt>& events,
                      AudioBuffer& buffer) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (!current_tick_) {
    current_tick_ = tick;
  }

  midi_stack_.AddEvents(events);

  if (!buffers_.empty()) {
    buffer = buffers_.front();
    buffers_.pop_front();
  }
}

}  // namespace inst
}  // namespace soir
