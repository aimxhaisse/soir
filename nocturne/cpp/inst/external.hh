#pragma once

#include <absl/status/status.h>

#include <condition_variable>
#include <libremidi/libremidi.hpp>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "audio/audio_buffer.hh"
#include "core/midi_stack.hh"
#include "inst/instrument.hh"

#define MA_NO_DECODING
#define MA_NO_ENCODING

#include <miniaudio.h>

namespace soir {
namespace inst {

class External : public Instrument {
 public:
  External();
  ~External();

  absl::Status Init(const std::string& settings, SampleManager* sample_manager,
                    Controls* controls);

  Type GetType() const { return Type::EXTERNAL; }
  std::string GetName() const { return "External"; }

  absl::Status Start();
  absl::Status Run();
  absl::Status Stop();

  void Render(SampleTick tick, const std::list<MidiEventAt>& events,
              AudioBuffer& buffer);

  static absl::Status GetMidiDevices(
      std::vector<std::pair<int, std::string>>* out);

 private:
  void ScheduleMidiEvents(const absl::Time& next_block_at);
  void WaitForInitialTick();

  absl::Status ParseAndValidateSettings(
      const std::string& settings, std::optional<std::string>* midi_out_device,
      std::optional<std::string>* audio_in_device, std::vector<int>* channels);
  absl::Status ConfigureMidiPort(
      const std::optional<std::string>& midi_out_device);
  absl::Status ConfigureAudioDevice(
      const std::optional<std::string>& audio_in_device,
      const std::vector<int>& channels);

  std::string settings_ = "";
  std::optional<std::string> settings_midi_out_ = std::nullopt;
  std::optional<std::string> settings_audio_in_ = std::nullopt;
  std::vector<int> settings_chans_ = {0, 1};

  std::mutex mutex_;
  std::condition_variable cv_;
  std::thread thread_;
  bool stop_ = false;
  std::vector<float> consumed_;
  std::list<AudioBuffer> buffers_;
  SampleTick current_tick_ = 0;

  libremidi::midi_out midi_out_;
  int audio_in_chans_ = -1;
  ma_context audio_in_context_;
  ma_device audio_in_device_;
  ma_pcm_rb audio_ringbuffer_;
  bool audio_in_context_initialized_ = false;
  bool audio_in_device_initialized_ = false;
  std::vector<int> channel_map_;

  void ProcessAudioInput();

  static void AudioInputCallback(ma_device* device, void* output,
                                 const void* input, ma_uint32 frame_count);

  MidiStack midi_stack_;
};

}  // namespace inst
}  // namespace soir
