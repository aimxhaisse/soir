#pragma once

#include <absl/status/status.h>
#include <condition_variable>
#include <libremidi/libremidi.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <thread>
#include <vector>

#include "core/audio_buffer.hh"
#include "core/inst/instrument.hh"
#include "core/midi_stack.hh"

namespace soir {
namespace inst {

class MidiExt : public Instrument {
 public:
  MidiExt();
  ~MidiExt();

  absl::Status Init(const std::string& settings, SampleManager* sample_manager,
                    Controls* controls);

  Type GetType() const { return Type::MIDI_EXT; }
  std::string GetName() const { return "MidiExt"; }

  absl::Status Start();
  absl::Status Run();
  absl::Status Stop();

  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer);

  // Get the list of available MIDI output devices (device number and associated name).
  static absl::Status GetMidiDevices(
      std::vector<std::pair<int, std::string>>* out);

 private:
  void ScheduleMidiEvents(const absl::Time& next_block_at);
  void FillAudioBuffer(const float* stream, size_t frames);
  void WaitForInitialTick();

  // Helper methods for Init function
  absl::Status ParseAndValidateSettings(const std::string& settings,
                                        std::string* midi_out_device,
                                        std::string* audio_in_device,
                                        std::vector<int>* channels);
  absl::Status ConfigureMidiPort(const std::string& midi_out_device);
  absl::Status ConfigureAudioDevice(const std::string& audio_in_device,
                                   const std::vector<int>& channels);

  static int AudioInputCallback(const void* in, void* out,
                               unsigned long frames,
                               const PaStreamCallbackTimeInfo* time,
                               PaStreamCallbackFlags flags,
                               void* user_data);

  struct PaStreamDeleter {
    void operator()(PaStream* stream);
  };

  // Current configuration as set from live coding. This is a cache
  // used to know upon update if we need to re-initialize the
  // device/chans, etc.
  std::string settings_ = "";
  std::string settings_midi_out_ = "";
  std::string settings_audio_in_ = "";
  std::vector<int> settings_chans_ = {0, 1};

  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread thread_;
  bool stop_ = false;
  std::vector<float> consumed_;
  std::list<AudioBuffer> buffers_;
  SampleTick current_tick_ = 0;

  // Underlying devices to which we are connected to.
  libremidi::midi_out midi_out_;
  std::unique_ptr<PaStream, PaStreamDeleter> audio_stream_;
  int audio_in_chans_ = -1;

  MidiStack midi_stack_;
};

}  // namespace inst
}  // namespace soir
