#include <absl/log/log.h>
#include <rapidjson/document.h>

#include "core/dsp.hh"
#include "core/inst/midi_ext.hh"
#include "core/sdl.hh"

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

MidiExt::MidiExt() {
  sdl::ListAudioInDevices();
}

MidiExt::~MidiExt() {
  if (audio_stream_) {
    SDL_DestroyAudioStream(audio_stream_);
    audio_stream_ = nullptr;
  }
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

  if (settings == settings_) {
    return absl::OkStatus();
  }

  rapidjson::Document params;
  params.Parse(settings.c_str());

  auto settings_midi_out = params["midi_out"].GetString();
  if (settings_midi_out != settings_midi_out_) {
    midi_out_.close_port();
    LOG(INFO) << "Trying to open MIDI port " << settings_midi_out << "...";

    if (!useMidiPort(settings_midi_out, midi_out_)) {
      LOG(WARNING) << "Failed to open MIDI port " << settings_midi_out;
      return absl::OkStatus();
    }
    settings_midi_out_ = settings_midi_out;
  }

  std::vector<int> chans;
  auto audio_chans = params["audio_channels"].GetArray();
  for (int i = 0; i < audio_chans.Size(); ++i) {
    chans.push_back(audio_chans[i].GetInt());
  }
  if (chans.size() != 2) {
    LOG(WARNING) << "Audio channels must be an array of two integers";
    return absl::OkStatus();
  }
  const bool chans_changed = (settings_chans_ != chans);

  auto settings_audio_in = params["audio_in"].GetString();
  if (settings_audio_in != settings_audio_in_ || chans_changed) {
    if (audio_out_ != 0) {
      if (audio_stream_) {
        SDL_DestroyAudioStream(audio_stream_);
        audio_stream_ = nullptr;
      }
    }

    LOG(INFO) << "Trying to open audio device " << settings_audio_in << "...";

    // Get the list of recording devices
    int num_devices = 0;
    SDL_AudioDeviceID *devices = SDL_GetAudioRecordingDevices(&num_devices);
    
    if (num_devices <= 0) {
      SDL_free(devices);
      LOG(WARNING) << "No audio recording devices available";
      return absl::OkStatus();
    }
    
    // Find the device by name
    int device_index = -1;
    for (int i = 0; i < num_devices; i++) {
      const char* name = SDL_GetAudioDeviceName(devices[i]);
      if (name && settings_audio_in == name) {
        device_index = i;
        break;
      }
    }
    
    if (device_index == -1) {
      SDL_free(devices);
      LOG(WARNING) << "Audio device not found: " << settings_audio_in;
      return absl::OkStatus();
    }
    
    SDL_AudioDeviceID device_id = devices[device_index];
    SDL_free(devices);

    SDL_AudioSpec spec;
    SDL_zero(spec);

    const int highest_desired_chan = std::max(chans[0], chans[1]);

    spec.freq = kSampleRate;
    spec.format = SDL_AUDIO_F32;
    spec.channels = highest_desired_chan;

    // Custom callback data structure for SDL3
    AudioStreamCallbackData* callback_data = new AudioStreamCallbackData;
    callback_data->midi_ext = this;
    
    LOG(INFO) << "Opening audio device " << settings_audio_in << "...";

    SDL_AudioStreamCallback callback = [](void* userdata, SDL_AudioStream* stream,
                                         int additional_amount, int total_amount) {
      AudioStreamCallbackData* data = static_cast<AudioStreamCallbackData*>(userdata);
      MidiExt* midi = data->midi_ext;
      
      // In SDL3, we handle audio data differently with the callback
      // Instead of filling the buffer directly, we'll respond to the stream's needs
      if (additional_amount > 0) {
        // Allocate a temporary buffer for the requested audio data
        std::vector<Uint8> buffer(additional_amount);
        midi->FillAudioBuffer(buffer.data(), additional_amount);
        
        // Put the data into the stream
        SDL_PutAudioStreamData(stream, buffer.data(), additional_amount);
      }
    };

    audio_stream_ = SDL_OpenAudioDeviceStream(device_id, &spec, callback, callback_data);
    if (!audio_stream_) {
      delete callback_data;
      LOG(WARNING) << "Failed to open audio device stream: " << SDL_GetError();
      return absl::OkStatus();
    }

    audio_out_ = device_id;
    audio_out_chans_ = spec.channels;

    settings_audio_in_ = settings_audio_in;
    settings_chans_ = chans;

    SDL_ResumeAudioDevice(device_id);
  }

  settings_ = settings;

  return absl::OkStatus();
}

void MidiExt::FillAudioBuffer(Uint8* stream, int len) {
  consumed_.resize(consumed_.size() + len);
  memcpy(consumed_.data() + consumed_.size() - len, stream, len);

  // We expect two channels of float samples interleaved.
  const uint32_t want = audio_out_chans_ * kBlockSize * sizeof(float);
  if (consumed_.size() < want) {
    return;
  }

  float* audio = reinterpret_cast<float*>(consumed_.data());
  AudioBuffer out(kBlockSize);
  float* left = out.GetChannel(kLeftChannel);
  float* right = out.GetChannel(kRightChannel);
  for (int i = 0; i < kBlockSize; i++) {
    left[i] = audio[audio_out_chans_ * i + settings_chans_[0]];
    right[i] = audio[audio_out_chans_ * i + settings_chans_[1]];
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

  if (audio_stream_) {
    SDL_DestroyAudioStream(audio_stream_);
    audio_stream_ = nullptr;
  }
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
