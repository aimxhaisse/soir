#pragma once

#include <absl/status/status.h>
#include <httplib.h>

#include "core/audio_buffer.hh"
#include "core/vorbis_encoder.hh"
#include "utils/misc.hh"

namespace soir {


// Handles a long-lived HTTP stream, transcoding on the fly
// raw samples into the desired format.
class HttpStream : public SampleConsumer {
 public:
  HttpStream();
  virtual ~HttpStream();

  absl::Status PushAudioBuffer(AudioBuffer& samples) override;
  absl::Status Encode(httplib::DataSink& sink);

 private:
  std::mutex mutex_;
  std::condition_variable cond_;
  std::list<AudioBuffer> stream_;
  bool initialized_;
  VorbisEncoder encoder_;
};


}  // namespace soir
