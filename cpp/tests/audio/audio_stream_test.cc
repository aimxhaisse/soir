#include "audio/audio_stream.hh"

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>

#include "audio/audio_buffer.hh"
#include "core/common.hh"

namespace soir {

TEST(AudioStreamTest, EncoderThreadProducesData) {
  audio::AudioStream stream;
  auto status = stream.Init(48000, 2, 128000);
  ASSERT_TRUE(status.ok());

  AudioBuffer buffer(kBlockSize);
  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);

  // Push ~1 s of a 440 Hz tone in 512-frame blocks. Silence is
  // pointless here: Opus encodes all-zero frames to 0 bytes and such
  // frames never flush an Ogg page.
  for (int block = 0; block < kSampleRate / kBlockSize; block++) {
    for (size_t i = 0; i < buffer.Size(); i++) {
      int n = block * kBlockSize + static_cast<int>(i);
      float v = 0.3f * sinf(2.0 * 3.14159265f * 440.0f * n / kSampleRate);
      left[i] = v;
      right[i] = v;
    }
    status = stream.PushAudioBuffer(buffer);
    ASSERT_TRUE(status.ok());
    std::this_thread::sleep_for(std::chrono::microseconds(1000));
  }

  // The header is seeded on init; after ~1 s of audio the ring must
  // contain encoded Opus data beyond it.
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  auto result = stream.Read(0, 100);
  EXPECT_FALSE(result.data.empty());
  EXPECT_GT(result.new_offset, 200u);

  stream.Stop();
}

}  // namespace soir
