#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp.hh"
#include "core/inst/midi_ext.hh"

namespace {

bool useMidiPort(int port, libremidi::midi_out& libremidi) {
  auto ports =
      libremidi::observer{
          {}, observer_configuration_for(libremidi.get_current_api())}
          .get_output_ports();

  if (port >= ports.size()) {
    LOG(ERROR) << "MIDI port number out of range!";
    return false;
  }

  LOG(INFO) << "Opening port " << port << ": " << ports[port].display_name;

  libremidi.open_port(ports[port]);

  if (!libremidi.is_port_open()) {
    LOG(ERROR) << "Failed to open MIDI port " << port;
    return false;
  }

  return true;
}

void printAudioDevices() {
  LOG(INFO) << "Available audio devices:";
  const int count = SDL_GetNumAudioDevices(1);
  for (int i = 0; i < count; ++i) {
    LOG(INFO) << "Audio device " << i << ": " << SDL_GetAudioDeviceName(i, 1);
  }
}

}  // namespace

namespace soir {
namespace inst {

MidiExt::MidiExt() {
  printAudioDevices();
}

MidiExt::~MidiExt() {}

absl::Status MidiExt::Init(const std::string& settings,
                           SampleManager* sample_manager, Controls* controls) {
  std::lock_guard<std::mutex> lock(mutex_);

  if (settings == current_settings_) {
    return absl::OkStatus();
  }

  rapidjson::Document params;
  params.Parse(settings.c_str());

  auto midi_port = params["midi_device"].GetInt();
  if (midi_port != current_midi_port_ && midi_port >= 0) {
    midiout_.close_port();
    LOG(INFO) << "Trying to open MIDI port " << midi_port << "...";

    if (!useMidiPort(midi_port, midiout_)) {
      return absl::InternalError("Failed to use MIDI port");
    }
    current_midi_port_ = midi_port;
  }

  auto audio_device = params["audio_device"].GetInt();
  if (audio_device != current_audio_device_) {
    if (current_audio_device_ != -1) {
      SDL_CloseAudioDevice(audio_device_id_);
    }

    LOG(INFO) << "Trying to open audio device " << audio_device << "...";
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

    const char* device_name = SDL_GetAudioDeviceName(audio_device, 1);

    LOG(INFO) << "Opening audio device " << device_name;

    audio_device_id_ = SDL_OpenAudioDevice(device_name, 1, &want, &have, 0);
    if (!(audio_device_id_ > 0)) {
      LOG(ERROR) << "Failed to open audio device: " << SDL_GetError();
      return absl::InternalError("Failed to open audio device");
    }

    current_audio_device_ = audio_device;

    SDL_PauseAudioDevice(audio_device_id_, 0);
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
      if (current_midi_port_ != -1) {
        for (auto& ev : events_at) {
          midiout_.send_message(ev.Msg());
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

  if (audio_device_id_ != -1) {
    SDL_CloseAudioDevice(audio_device_id_);
  }
  if (midiout_.is_port_open()) {
    midiout_.close_port();
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
