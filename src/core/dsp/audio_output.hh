#pragma once

#include <RtAudio.h>
#include <absl/status/status.h>
#include <condition_variable>
#include <list>
#include <mutex>

#include "core/dsp/dsp.hh"
#include "utils/config.hh"

namespace neon {
namespace dsp {

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

  absl::Status PushAudioBuffer(const AudioBuffer& buffer) override;

 private:
  absl::Status Consume(float* out, unsigned int n);

  std::mutex mutex_;
  std::condition_variable cond_;
  std::list<AudioBuffer> stream_;
  bool stop_ = false;

  unsigned int buffer_size_ = 0;
  std::unique_ptr<RtAudio> audio_;
  int position_ = 0;
  AudioBuffer current_;
};

}  // namespace dsp
}  // namespace neon
