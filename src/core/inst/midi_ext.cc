#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp.hh"
#include "core/inst/midi_ext.hh"

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

void printAudioDevices() {
  LOG(INFO) << "Available audio input devices:";
  const int count = SDL_GetNumAudioDevices(1);
  for (int i = 0; i < count; ++i) {
    LOG(INFO) << "Audio input device " << i << ": "
              << SDL_GetAudioDeviceName(i, 1);
  }
}

}  // namespace

namespace soir {
namespace inst {

MidiExt::MidiExt() {
  printAudioDevices();
}

MidiExt::~MidiExt() {}

absl::Status MidiExt::GetAudioDevices(
    std::vector<std::pair<int, std::string>>* out) {
  int count = SDL_GetNumAudioDevices(1);
  for (int i = 0; i < count; ++i) {
    const char* name = SDL_GetAudioDeviceName(i, 1);
    if (name) {
      out->emplace_back(i, name);
    }

    SDL_AudioSpec spec;
    if (SDL_GetAudioDeviceSpec(i, 1, &spec) == 0) {
      LOG(INFO) << "Audio device " << i << ": " << name
                << ", freq: " << spec.freq << ", format: " << spec.format
                << ", channels: " << static_cast<int>(spec.channels)
                << ", samples: " << spec.samples;
    } else {
      LOG(WARNING) << "Failed to get audio device spec for device " << i;
    }
  }

  return absl::OkStatus();
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

absl::Status MidiExt::Init(const std::string& settings,
                           SampleManager* sample_manager, Controls* controls) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings == current_settings_) {
    return absl::OkStatus();
  }

  rapidjson::Document params;
  params.Parse(settings.c_str());

  auto midi_out = params["midi_out"].GetString();
  if (midi_out != current_midi_out_) {
    active_midi_out_.close_port();
    LOG(INFO) << "Trying to open MIDI port " << midi_out << "...";

    if (!useMidiPort(midi_out, active_midi_out_)) {
      LOG(WARNING) << "Failed to open MIDI port " << midi_out;
      return absl::OkStatus();
    }
    current_midi_out_ = midi_out;
  }

  std::vector<int> channels;
  auto audio_channels = params["audio_channels"].GetArray();
  for (int i = 0; i < audio_channels.Size(); ++i) {
    channels.push_back(channel.GetInt());
  }

  auto audio_in = params["audio_in"].GetString();
  if (audio_in != current_audio_in_) {
    if (active_audio_out_ != -1) {
      SDL_CloseAudioDevice(active_audio_out_);
    }

    LOG(INFO) << "Trying to open audio device " << audio_in << "...";

    SDL_AudioSpec want, have;
    SDL_zero(want);

    want.freq = kSampleRate;
    want.format = AUDIO_F32SYS;
    want.channels = 2;
    want.samples = kBlockSize;
    want.userdata = this;

    want.callback = [](void* userdata, Uint8* stream, int len) {
      MidiExt* midi = static_cast<MidiExt*>(userdata);
      midi->FillAudioBuffer(stream, len);
    };

    LOG(INFO) << "Opening audio device " << audio_in << "...";

    active_audio_out_ = SDL_OpenAudioDevice(audio_in, 1, &want, &have, 0);
    if (!(active_audio_out_ > 0)) {
      LOG(WARNING) << "Failed to open audio device: " << SDL_GetError();
      return absl::OkStatus();
    }

    current_audio_in_ = audio_in;

    SDL_PauseAudioDevice(active_audio_out_, 0);
  }

  current_settings_ = settings;

  return absl::OkStatus();
}

void MidiExt::FillAudioBuffer(Uint8* stream, int len) {
  consumed_.resize(consumed_.size() + len);
  memcpy(consumed_.data() + consumed_.size() - len, stream, len);

  // We expect two channels of float samples interleaved.
  const uint32_t want = 2 * kBlockSize * sizeof(float);
  if (consumed_.size() < want) {
    return;
  }

  float* audio = reinterpret_cast<float*>(consumed_.data());
  AudioBuffer out(kBlockSize);
  float* left = out.GetChannel(kLeftChannel);
  float* right = out.GetChannel(kRightChannel);
  for (int i = 0; i < kBlockSize; i++) {
    left[i] = audio[2 * i];
    right[i] = audio[2 * i + 1];
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
      if (!current_midi_out_.empty()) {
        for (auto& ev : events_at) {
          active_midi_out_.send_message(ev.Msg());
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

  if (active_audio_out_ != -1) {
    SDL_CloseAudioDevice(active_audio_out_);
  }
  if (active_midi_out_.is_port_open()) {
    active_midi_out_.close_port();
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
