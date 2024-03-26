#pragma once

#include <absl/status/status.h>
#include <httplib.h>

#include "audio_buffer.hh"
#include "utils/misc.hh"
#include "vorbis_encoder.hh"

namespace neon {
namespace dsp {

// Handles a long-lived HTTP stream, transcoding on the fly
// raw samples into the desired format.
class HttpStream : public SampleConsumer {
 public:
  HttpStream();
  virtual ~HttpStream();

  absl::Status PushAudioBuffer(const AudioBuffer& samples) override;
  absl::Status Encode(httplib::DataSink& sink);

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::list<AudioBuffer> stream_;
  bool initialized_;
  VorbisEncoder encoder_;
};

}  // namespace dsp
}  // namespace neon
