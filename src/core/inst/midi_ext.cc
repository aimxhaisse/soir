#include <absl/log/log.h>
#include <absl/status/status.h>
#include <rapidjson/document.h>

#include "core/dsp.hh"
#include "core/inst/midi_ext.hh"
#include "core/portaudio.hh"

namespace {

bool useMidiPort(const std::string midi_out, libremidi::midi_out& libremidi) {
  auto ports =
      libremidi::observer{
          {}, observer_configuration_for(libremidi.get_current_api())}
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

void MidiExt::PaStreamDeleter::operator()(PaStream* stream) {
  if (stream) {
    Pa_CloseStream(stream);
  }
}

MidiExt::MidiExt() {
  portaudio::ListAudioInDevices();
}

MidiExt::~MidiExt() {
  audio_stream_.reset();
}

absl::Status MidiExt::GetMidiDevices(
    std::vector<std::pair<int, std::string>>* out) {
  libremidi::midi_out midi_out;
  auto ports =
      libremidi::observer{
          {}, observer_configuration_for(midi_out.get_current_api())}
          .get_output_ports();
  int i = 0;
  for (auto& port : ports) {
    out->emplace_back(i, port.display_name);
    i++;
  }
  return absl::OkStatus();
}

absl::Status MidiExt::ParseAndValidateSettings(const std::string& settings,
                                               std::string* midi_out_device,
                                               std::string* audio_in_device,
                                               std::vector<int>* channels) {
  rapidjson::Document params;
  params.Parse(settings.c_str());

  if (!params.HasMember("midi_out") || !params["midi_out"].IsString()) {
    return absl::InvalidArgumentError("Missing or invalid 'midi_out' field");
  }
  *midi_out_device = params["midi_out"].GetString();

  if (!params.HasMember("audio_in") || !params["audio_in"].IsString()) {
    return absl::InvalidArgumentError("Missing or invalid 'audio_in' field");
  }
  *audio_in_device = params["audio_in"].GetString();

  if (!params.HasMember("audio_channels") ||
      !params["audio_channels"].IsArray()) {
    return absl::InvalidArgumentError(
        "Missing or invalid 'audio_channels' field");
  }

  auto audio_chans = params["audio_channels"].GetArray();
  channels->clear();
  for (int i = 0; i < audio_chans.Size(); ++i) {
    if (!audio_chans[i].IsInt()) {
      return absl::InvalidArgumentError("Audio channels must be integers");
    }
    channels->push_back(audio_chans[i].GetInt());
  }

  if (channels->size() != 2) {
    return absl::InvalidArgumentError(
        "Audio channels must be an array of two integers");
  }

  return absl::OkStatus();
}

absl::Status MidiExt::ConfigureMidiPort(const std::string& midi_out_device) {
  if (midi_out_device == settings_midi_out_) {
    return absl::OkStatus();
  }

  midi_out_.close_port();
  LOG(INFO) << "Trying to open MIDI port " << midi_out_device << "...";

  if (!useMidiPort(midi_out_device, midi_out_)) {
    LOG(WARNING) << "Failed to open MIDI port " << midi_out_device;
    return absl::OkStatus();
  }

  settings_midi_out_ = midi_out_device;
  return absl::OkStatus();
}

absl::Status MidiExt::ConfigureAudioDevice(const std::string& audio_in_device,
                                           const std::vector<int>& channels) {
  const bool chans_changed = (settings_chans_ != channels);
  const bool device_changed = (audio_in_device != settings_audio_in_);

  if (!device_changed && !chans_changed) {
    return absl::OkStatus();
  }

  // Clean up existing audio stream if necessary
  audio_stream_.reset();

  LOG(INFO) << "Trying to open audio device " << audio_in_device << "...";

  // Get the list of recording devices
  std::vector<portaudio::Device> devices;
  auto status = portaudio::GetAudioInDevices(&devices);
  if (!status.ok()) {
    LOG(WARNING) << "Failed to get audio input devices: " << status;
    return absl::OkStatus();
  }

  if (devices.empty()) {
    LOG(WARNING) << "No audio recording devices available";
    return absl::OkStatus();
  }

  // Find the device by name
  int device_index = -1;
  for (size_t i = 0; i < devices.size(); i++) {
    if (devices[i].name == audio_in_device) {
      device_index = devices[i].id;
      break;
    }
  }

  if (device_index == -1) {
    LOG(WARNING) << "Audio device not found: " << audio_in_device;
    return absl::OkStatus();
  }

  const int highest_desired_chan = std::max(channels[0], channels[1]) + 1;

  LOG(INFO) << "Opening audio device " << audio_in_device << " with "
            << highest_desired_chan << " channels at " << kSampleRate
            << " Hz, format: float32";

  PaStreamParameters inputParameters;
  inputParameters.device = device_index;
  inputParameters.channelCount = highest_desired_chan;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(device_index)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = nullptr;

  PaStream* raw_stream = nullptr;
  PaError err = Pa_OpenStream(&raw_stream,
                             &inputParameters,
                             nullptr,  // no output
                             kSampleRate,
                             kBlockSize,
                             paNoFlag,
                             AudioInputCallback,
                             this);

  if (err != paNoError) {
    LOG(WARNING) << "Failed to open audio input stream: " << Pa_GetErrorText(err);
    return absl::OkStatus();
  }

  err = Pa_StartStream(raw_stream);
  if (err != paNoError) {
    Pa_CloseStream(raw_stream);
    LOG(WARNING) << "Failed to start audio input stream: " << Pa_GetErrorText(err);
    return absl::OkStatus();
  }

  // Update state
  audio_stream_.reset(raw_stream);
  audio_in_chans_ = highest_desired_chan;
  settings_audio_in_ = audio_in_device;
  settings_chans_ = channels;

  return absl::OkStatus();
}

absl::Status MidiExt::Init(const std::string& settings,
                           SampleManager* sample_manager, Controls* controls) {
  std::lock_guard<std::mutex> lock(mutex_);

  // Early return if settings haven't changed
  if (settings == settings_) {
    return absl::OkStatus();
  }

  // Parse and validate settings
  std::string midi_out_device;
  std::string audio_in_device;
  std::vector<int> channels;

  auto status = ParseAndValidateSettings(settings, &midi_out_device,
                                         &audio_in_device, &channels);
  if (!status.ok()) {
    LOG(WARNING) << "Invalid settings: " << status.message();
    return absl::OkStatus();
  }

  // Configure MIDI port
  status = ConfigureMidiPort(midi_out_device);
  if (!status.ok()) {
    return status;
  }

  // Configure audio device
  status = ConfigureAudioDevice(audio_in_device, channels);
  if (!status.ok()) {
    return status;
  }

  // Update settings cache
  settings_ = settings;
  return absl::OkStatus();
}

int MidiExt::AudioInputCallback(const void* in, void* out,
                               unsigned long frames,
                               const PaStreamCallbackTimeInfo* time,
                               PaStreamCallbackFlags flags,
                               void* user_data) {
  auto* midi_ext = static_cast<MidiExt*>(user_data);
  auto* input = static_cast<const float*>(in);
  
  midi_ext->FillAudioBuffer(input, frames);
  return paContinue;
}

void MidiExt::FillAudioBuffer(const float* stream, size_t frames) {
  size_t samples_to_add = frames * audio_in_chans_;
  size_t current_size = consumed_.size();
  consumed_.resize(current_size + samples_to_add);
  
  // Copy the interleaved float data
  std::memcpy(consumed_.data() + current_size, stream, samples_to_add * sizeof(float));

  // We expect float samples interleaved.
  const size_t want = audio_in_chans_ * kBlockSize;
  if (consumed_.size() < want) {
    return;
  }

  AudioBuffer out(kBlockSize);
  float* left = out.GetChannel(kLeftChannel);
  float* right = out.GetChannel(kRightChannel);
  
  for (size_t i = 0; i < kBlockSize; i++) {
    left[i] = consumed_[audio_in_chans_ * i + settings_chans_[0]];
    right[i] = consumed_[audio_in_chans_ * i + settings_chans_[1]];
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    buffers_.push_back(out);
  }

  consumed_.erase(consumed_.begin(), consumed_.begin() + want);
}

void MidiExt::ScheduleMidiEvents(const absl::Time& block_at) {
  // We only fetch events for the current block once per block,
  // this avoids taking too much time locks in the critical path.
  uint32_t current_tick;
  MidiStack events;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<MidiEventAt> events_at;
    midi_stack_.EventsAtTick(current_tick_ + kBlockSize, events_at);
    events.AddEvents(events_at);
    current_tick = current_tick_;
  }

  // Here we spread MIDI events with a precision of kMidiExtChunkSize
  // samples.  This is to avoid sleeping on each sample and it leaves
  // some extra time on the last chunk to fill the audio buffer.
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
      if (!settings_midi_out_.empty()) {
        for (auto& ev : events_at) {
          midi_out_.send_message(ev.Msg());
        }
      }
    }

    chunk += 1;
  } while ((chunk * nsamples) < kBlockSize);
}

absl::Status MidiExt::Start() {
  LOG(INFO) << "Starting MIDI thread";

  thread_ = std::thread([this]() {
    auto status = Run();
    if (!status.ok()) {
      LOG(ERROR) << "MIDI thread failed: " << status;
    }
  });

  return absl::OkStatus();
}

void MidiExt::WaitForInitialTick() {
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

absl::Status MidiExt::Run() {
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

    block_count += 1;
    next_block_at = initial_time + (block_count * block_duration);
    current_tick_ += kBlockSize;
  }

  return absl::OkStatus();
}

absl::Status MidiExt::Stop() {
  LOG(INFO) << "Stopping MIDI thread";

  {
    std::unique_lock<std::mutex> lock(mutex_);
    stop_ = true;
    cv_.notify_all();
  }

  thread_.join();

  LOG(INFO) << "MIDI thread stopped";

  audio_stream_.reset();
  if (midi_out_.is_port_open()) {
    midi_out_.close_port();
  }

  return absl::OkStatus();
}

void MidiExt::Render(SampleTick tick, const std::list<MidiEventAt>& events,
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
