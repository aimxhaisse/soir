#pragma once

#include <absl/status/status.h>
#include <memory>
#include <mutex>
#include <portaudio.h>
#include <vector>

#include "core/dsp.hh"
#include "utils/config.hh"

namespace soir {

// This class consumes samples from the DSP engine and output them to
// the audio device. The first audio device is currently selected but
// we could later imagine configure it from code.
class AudioOutput : public SampleConsumer {
 public:
  AudioOutput();
  virtual ~AudioOutput();

  absl::Status Init(const utils::Config& config);
  absl::Status Start();
  absl::Status Stop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

 private:
  static int AudioCallback(const void* inputBuffer, void* outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

  struct PaStreamDeleter {
    void operator()(PaStream* stream);
  };

  std::unique_ptr<PaStream, PaStreamDeleter> stream_;
  std::vector<float> ring_buffer_;
  size_t ring_buffer_write_pos_ = 0;
  size_t ring_buffer_read_pos_ = 0;
  size_t ring_buffer_size_ = 0;
  std::mutex buffer_mutex_;
};

}  // namespace soir
