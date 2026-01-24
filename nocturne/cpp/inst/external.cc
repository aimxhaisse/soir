#include "inst/external.hh"

#include <absl/log/log.h>
#include <absl/status/status.h>
#include <rapidjson/document.h>

#include "core/common.hh"

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

  const float* input_buffer = static_cast<const float*>(input);
  ma_pcm_rb_acquire_write(&ext->audio_ringbuffer_, &frame_count,
                          (void**)&input_buffer);
  ma_pcm_rb_commit_write(&ext->audio_ringbuffer_, frame_count);
}

void External::ProcessAudioInput() {
  if (!audio_in_device_initialized_) {
    return;
  }

  ma_uint32 available_frames = ma_pcm_rb_available_read(&audio_ringbuffer_);
  if (available_frames < kBlockSize) {
    return;
  }

  std::vector<float> input_buffer(kBlockSize * audio_in_chans_);
  void* read_ptr;
  ma_uint32 frames_to_read = kBlockSize;
  ma_pcm_rb_acquire_read(&audio_ringbuffer_, &frames_to_read, &read_ptr);
  if (frames_to_read < kBlockSize) {
    ma_pcm_rb_commit_read(&audio_ringbuffer_, frames_to_read);
    return;
  }

  memcpy(input_buffer.data(), read_ptr,
         kBlockSize * audio_in_chans_ * sizeof(float));
  ma_pcm_rb_commit_read(&audio_ringbuffer_, kBlockSize);

  AudioBuffer output_buffer(kBlockSize);

  int left_channel = channel_map_[0];
  int right_channel = channel_map_[1];
  float* left = output_buffer.GetChannel(kLeftChannel);
  float* right = output_buffer.GetChannel(kRightChannel);

  for (int i = 0; i < kBlockSize; ++i) {
    left[i] = input_buffer[i * audio_in_chans_ + left_channel];
    right[i] = input_buffer[i * audio_in_chans_ + right_channel];
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    buffers_.push_back(output_buffer);
  }
}

External::External() { audio_in_device_initialized_ = false; }

External::~External() {
  if (audio_in_device_initialized_) {
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
  }
}

absl::Status External::GetMidiDevices(
    std::vector<std::pair<int, std::string>>* out) {
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
  rapidjson::Document params;
  params.Parse(settings.c_str());

  if (params.HasMember("midi_out") && params["midi_out"].IsString()) {
    *midi_out_device = params["midi_out"].GetString();
  } else {
    *midi_out_device = std::nullopt;
  }

  if (params.HasMember("audio_in") && params["audio_in"].IsString()) {
    *audio_in_device = params["audio_in"].GetString();
  } else {
    *audio_in_device = std::nullopt;
  }

  if (!midi_out_device->has_value() && !audio_in_device->has_value()) {
    return absl::InvalidArgumentError(
        "At least one of midi_out or audio_in must be specified");
  }

  if (audio_in_device->has_value()) {
    if (!params.HasMember("audio_channels") ||
        !params["audio_channels"].IsArray()) {
      return absl::InvalidArgumentError(
          "audio_channels required when audio_in is set");
    }

    auto audio_chans = params["audio_channels"].GetArray();
    if (audio_chans.Size() != 2) {
      return absl::InvalidArgumentError(
          "audio_channels must have exactly 2 elements [L, R]");
    }

    channels->clear();
    for (rapidjson::SizeType i = 0; i < audio_chans.Size(); ++i) {
      if (!audio_chans[i].IsInt()) {
        return absl::InvalidArgumentError("Audio channels must be integers");
      }
      channels->push_back(audio_chans[i].GetInt());
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

  if (midi_out_device == settings_midi_out_) {
    return absl::OkStatus();
  }

  midi_out_.close_port();
  LOG(INFO) << "Trying to open MIDI port " << *midi_out_device << "...";

  if (!useMidiPort(*midi_out_device, midi_out_)) {
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

  if (!device_changed && !chans_changed) {
    return absl::OkStatus();
  }

  if (audio_in_device_initialized_) {
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
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

  ma_uint32 ringbuffer_size = kBlockSize * 4 * required_channels;
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
  config.dataCallback = AudioInputCallback;
  config.pUserData = this;

  ma_device_info* capture_devices;
  ma_uint32 capture_device_count;
  ma_context context;

  result = ma_context_init(nullptr, 0, nullptr, &context);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to initialize audio context: " << result;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  result = ma_context_get_devices(&context, &capture_devices,
                                  &capture_device_count, nullptr, nullptr);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to enumerate audio devices: " << result;
    ma_context_uninit(&context);
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
    ma_context_uninit(&context);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  result = ma_device_init(&context, &config, &audio_in_device_);
  ma_context_uninit(&context);

  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to open audio device: " << result;
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    return absl::OkStatus();
  }

  result = ma_device_start(&audio_in_device_);
  if (result != MA_SUCCESS) {
    LOG(WARNING) << "Failed to start audio device: " << result;
    ma_device_uninit(&audio_in_device_);
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

void External::ScheduleMidiEvents(const absl::Time& block_at) {
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
  const float nus =
      (static_cast<float>(nsamples) / static_cast<float>(kSampleRate)) * 1e6;

  int chunk = 0;
  do {
    absl::Time chunk_at = block_at + absl::Microseconds(chunk * nus);
    std::this_thread::sleep_until(absl::ToChronoTime(chunk_at));
    std::list<MidiEventAt> events_at;
    events.EventsAtTick(current_tick + (1 + chunk) * nsamples, events_at);
    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (settings_midi_out_.has_value()) {
        for (auto& ev : events_at) {
          midi_out_.send_message(ev.Msg());
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

  absl::Duration block_duration =
      absl::Microseconds((1e6 * kBlockSize) / kSampleRate);
  absl::Time next_block_at = absl::Now();
  absl::Time initial_time = next_block_at;
  uint32_t block_count = 0;

  while (true) {
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait_until(lock, absl::ToChronoTime(next_block_at),
                     [this]() { return stop_; });
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

  thread_.join();

  LOG(INFO) << "External thread stopped";

  if (audio_in_device_initialized_) {
    ma_device_stop(&audio_in_device_);
    ma_device_uninit(&audio_in_device_);
    ma_pcm_rb_uninit(&audio_ringbuffer_);
    audio_in_device_initialized_ = false;
  }

  if (midi_out_.is_port_open()) {
    midi_out_.close_port();
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
