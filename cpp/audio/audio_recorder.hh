#pragma once

#include <absl/status/status.h>

#include <atomic>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "audio/audio_buffer.hh"
#include "core/common.hh"
#include "readerwriterqueue.h"

namespace soir {

// AudioRecorder consumes audio samples and writes them to a WAV file.
// Samples go through a queue to a dedicated writer thread, so disk I/O
// never runs on the engine thread and cannot stall the audio output.
class AudioRecorder : public SampleConsumer {
 public:
  AudioRecorder();
  ~AudioRecorder() override;

  absl::Status Init(const std::string& file_path);
  absl::Status MaybeStop();

  absl::Status PushAudioBuffer(AudioBuffer& buffer) override;

 private:
  void WriterLoop();
  bool WriteChunk(const std::vector<float>& chunk);
  static void WriteWavHeader(std::ofstream& out, uint64_t frames);

  std::string file_path_;
  std::atomic<bool> is_recording_;
  std::atomic<bool> stop_;
  std::atomic<bool> write_failed_;
  std::ofstream file_;
  std::thread writer_thread_;

  // Engine (sole producer) -> writer thread (sole consumer).
  moodycamel::BlockingReaderWriterQueue<float> queue_;

  // Written by the writer thread, read after the thread is joined.
  uint64_t total_frames_;
};

}  // namespace soir
