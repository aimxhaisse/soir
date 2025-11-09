#include "audio/audio_buffer.hh"

#include <gtest/gtest.h>

#include "dsp/dsp.hh"

namespace soir {

TEST(AudioBufferTest, Construction) {
  AudioBuffer buffer(1024);
  EXPECT_EQ(buffer.Size(), 1024);
}

TEST(AudioBufferTest, GetChannels) {
  AudioBuffer buffer(512);
  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);

  EXPECT_NE(left, nullptr);
  EXPECT_NE(right, nullptr);
  EXPECT_NE(left, right);
}

TEST(AudioBufferTest, Reset) {
  AudioBuffer buffer(256);

  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);

  // Fill with non-zero values
  for (int i = 0; i < 256; ++i) {
    left[i] = 1.0f;
    right[i] = 2.0f;
  }

  buffer.Reset();

  // Check all zeros
  for (int i = 0; i < 256; ++i) {
    EXPECT_EQ(left[i], 0.0f);
    EXPECT_EQ(right[i], 0.0f);
  }
}

TEST(AudioBufferTest, CopyConstruction) {
  AudioBuffer buffer1(128);
  float* left1 = buffer1.GetChannel(kLeftChannel);
  left1[0] = 3.14f;

  AudioBuffer buffer2 = buffer1;
  float* left2 = buffer2.GetChannel(kLeftChannel);

  EXPECT_EQ(buffer2.Size(), 128);
  EXPECT_EQ(left2[0], 3.14f);
}

}  // namespace soir
