#pragma once

#include <AudioFile.h>
#include <absl/status/status.h>

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "audio/audio_buffer.hh"
#include "core/common.hh"

namespace soir {

// AudioRecorder consumes audio samples and writes them to a WAV file.
// It implements the SampleConsumer interface to receive audio data from
// the engine and outputs it in WAV format for end-to-end testing purposes.
class AudioRecorder : public SampleConsumer {
 public:
  AudioRecorder();
  virtual ~AudioRecorder();

  absl::Status Init(const std::string& file_path);
  absl::Status MaybeStop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

 private:
  std::string file_path_;
  std::mutex buffer_mutex_;
  bool is_recording_ = false;
  AudioFile<float> audio_file_;
};

}  // namespace soir