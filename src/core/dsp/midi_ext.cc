#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp/dsp.hh"
#include "core/dsp/midi_ext.hh"

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

namespace neon {
namespace dsp {

MidiExt::MidiExt(uint32_t block_size) : block_size_(block_size) {
  printAudioDevices();
}

MidiExt::~MidiExt() {}

absl::Status MidiExt::Init(const std::string& settings) {
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
    want.samples = block_size_;
    want.callback = nullptr;

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

void MidiExt::ConsumeAudioBuffer() {
  const uint32_t want = 2 * block_size_ * sizeof(float);
  char buffer[want];

  {
    while (true) {
      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (current_audio_device_ == -1) {
          return;
        }
        if (SDL_GetQueuedAudioSize(audio_device_id_) >= want) {
          if (SDL_DequeueAudio(audio_device_id_, buffer, want) != want) {
            LOG(ERROR) << "Dequeued less than expected audio buffer size";
          }
          break;
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
  }

  float* audio = reinterpret_cast<float*>(buffer);

  AudioBuffer out(block_size_);

  float* left = out.GetChannel(kLeftChannel);
  float* right = out.GetChannel(kRightChannel);

  for (int i = 0; i < block_size_; i++) {
    left[i] = audio[2 * i];
    right[i] = audio[2 * i + 1];
  }

  {
    std::lock_guard<std::mutex> lock(mutex_);
    buffers_.push_back(out);
  }
}

void MidiExt::ScheduleMidiEvents(const absl::Time& next_block_at) {
  // We only fetch events for the current block once per block,
  // this avoids taking too much time locks in the critical path.
  MidiStack events;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    std::list<MidiEventAt> events_at;
    midi_stack_.EventsAtTick(current_tick_ + block_size_, events_at);
    events.AddEvents(events_at);
  }

  // Here we spread MIDI events with a precision of around 100us,
  // this is to avoid sleeping on each sample and it leaves some
  // extra time on the last chunk to fill the audio buffer.
  static constexpr uint32_t kPrecisionUs = 1e6 / 1e4;
  static constexpr uint32_t kPrecisionSamples = (kSampleRate * 1e4) / 1e6;

  for (int i = 0; i < block_size_; i += kPrecisionSamples) {
    std::this_thread::sleep_until(absl::ToChronoTime(
        next_block_at + (i * absl::Microseconds(kPrecisionUs))));
    std::list<MidiEventAt> events_at;
    events.EventsAtTick(current_tick_ + i, events_at);

    {
      std::lock_guard<std::mutex> lock(mutex_);
      if (current_midi_port_ > 0) {
        for (auto& ev : events_at) {
          midiout_.send_message(ev.Msg());
        }
      }
    }
  }
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

absl::Status MidiExt::Run() {
  AudioBuffer buffer(block_size_);

  absl::Duration block_duration =
      absl::Microseconds((1e6 * block_size_) / kSampleRate);
  absl::Time next_block_at = absl::Now();

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
    ConsumeAudioBuffer();

    next_block_at += block_duration;
    current_tick_ += block_size_;
  }

  if (audio_device_id_ != -1) {
    SDL_CloseAudioDevice(audio_device_id_);
  }
  if (midiout_.is_port_open()) {
    midiout_.close_port();
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

}  // namespace dsp
}  // namespace neon
