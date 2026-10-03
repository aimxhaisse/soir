#include "audio/audio_output.hh"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <thread>
#include <vector>

#include "audio/audio_buffer.hh"

namespace soir {
namespace {

// A block whose samples encode the block index (1-based, so silence is
// distinguishable from real audio), letting the consumer tell which
// blocks survived and in which order.
AudioBuffer MakeBlock(int index) {
  AudioBuffer buffer(kBlockSize);
  float* left = buffer.GetChannel(kLeftChannel);
  float* right = buffer.GetChannel(kRightChannel);
  for (int i = 0; i < kBlockSize; i++) {
    left[i] = static_cast<float>(index + 1);
    right[i] = static_cast<float>(index + 1) + 0.5f;
  }
  return buffer;
}

// Pulls whole-block callbacks out of the device side and returns the
// block indices it saw, skipping silence.
std::vector<int> DrainBlocks(audio::AudioOutput& output, int callbacks) {
  std::vector<int> blocks;
  std::vector<float> out(kBlockSize * kNumChannels, 0.0f);
  for (int call = 0; call < callbacks; call++) {
    output.FillCallbackBuffer(out.data(), out.size());
    if (out[0] > 0.0f) {
      blocks.push_back(static_cast<int>(out[0]) - 1);
    }
  }
  return blocks;
}

}  // namespace

TEST(AudioOutputTest, Initialization) {
  audio::AudioOutput output;

  auto status = output.Init(48000, 2, 512);
  EXPECT_TRUE(status.ok());
}

TEST(AudioOutputTest, StartStop) {
  audio::AudioOutput output;

  auto init_status = output.Init(48000, 2, 512);
  EXPECT_TRUE(init_status.ok());

  auto start_status = output.Start();
  EXPECT_TRUE(start_status.ok());

  auto stop_status = output.Stop();
  EXPECT_TRUE(stop_status.ok());
}

TEST(AudioOutputTest, RejectsWrongChannelCount) {
  audio::AudioOutput output;
  EXPECT_FALSE(output.Init(48000, 1, 512).ok());
}

TEST(AudioOutputRingTest, DeliversEveryBlockInOrder) {
  audio::AudioOutput output;

  for (int i = 0; i < 8; i++) {
    AudioBuffer block = MakeBlock(i);
    EXPECT_TRUE(output.PushAudioBuffer(block).ok());
  }

  std::vector<int> blocks = DrainBlocks(output, 16);
  EXPECT_EQ(blocks.size(), 8u);
  EXPECT_TRUE(std::is_sorted(blocks.begin(), blocks.end()));
  EXPECT_EQ(blocks.front(), 0);
  EXPECT_EQ(blocks.back(), 7);
}

TEST(AudioOutputRingTest, HandlesCallbacksSmallerThanABlock) {
  audio::AudioOutput output;

  for (int i = 0; i < 4; i++) {
    AudioBuffer block = MakeBlock(i);
    EXPECT_TRUE(output.PushAudioBuffer(block).ok());
  }

  // A device period is a hint: callbacks may ask for any frame count.
  const size_t kCallbackSamples = 100 * kNumChannels;
  std::vector<float> out(kCallbackSamples, -1.0f);
  std::vector<float> stream;
  for (int call = 0; call < 64; call++) {
    output.FillCallbackBuffer(out.data(), kCallbackSamples);
    stream.insert(stream.end(), out.begin(), out.end());
  }

  int seen = -1;
  int transitions = 0;
  for (size_t i = 0; i + 1 < stream.size(); i += 2) {
    const float left = stream[i];
    const float right = stream[i + 1];
    if (left <= 0.0f) {
      continue;
    }
    EXPECT_EQ(right, left + 0.5f) << "torn stereo pair at " << i;
    if (left != seen) {
      EXPECT_GT(left, seen) << "blocks delivered out of order at " << i;
      seen = static_cast<int>(left);
      transitions++;
    }
  }
  EXPECT_EQ(transitions, 4);
}

TEST(AudioOutputRingTest, BacklogIsCappedAndKeepsTheNewestBlocks) {
  audio::AudioOutput output;

  // Simulate a long engine stall being fast-forwarded: nothing is
  // consumed while 200 blocks are pushed.
  const int kPushed = 200;
  for (int i = 0; i < kPushed; i++) {
    AudioBuffer block = MakeBlock(i);
    EXPECT_TRUE(output.PushAudioBuffer(block).ok());
  }

  std::vector<int> blocks = DrainBlocks(output, 2 * audio::kOutputSlotCount);

  ASSERT_FALSE(blocks.empty());
  EXPECT_LE(blocks.size(), static_cast<size_t>(audio::kOutputMaxBacklogSlots));
  EXPECT_EQ(blocks.back(), kPushed - 1);
  // Stale blocks are dropped as a contiguous run from the front, never
  // from the middle.
  EXPECT_EQ(blocks.front(),
            blocks.back() - static_cast<int>(blocks.size()) + 1);
}

TEST(AudioOutputRingTest, UnderrunInsertsSilence) {
  audio::AudioOutput output;

  AudioBuffer block = MakeBlock(0);
  EXPECT_TRUE(output.PushAudioBuffer(block).ok());

  std::vector<float> out(kBlockSize * kNumChannels, -1.0f);
  int silent_callbacks = 0;
  for (int call = 0; call < 16; call++) {
    output.FillCallbackBuffer(out.data(), out.size());
    if (out[0] == 0.0f) {
      silent_callbacks++;
    }
  }

  EXPECT_GT(silent_callbacks, 0);
  EXPECT_GT(output.GetUnderrunCount(), 0u);
}

TEST(AudioOutputRingTest, SurvivesConcurrentProducerAndConsumer) {
  audio::AudioOutput output;

  const int kPushed = 4000;
  std::thread producer([&output, kPushed]() {
    for (int i = 0; i < kPushed; i++) {
      AudioBuffer block = MakeBlock(i);
      EXPECT_TRUE(output.PushAudioBuffer(block).ok());
      // Roughly the engine's cadence, so the device thread keeps up and
      // the cap is not what is being tested here.
      std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
  });

  std::vector<float> out(kBlockSize * kNumChannels, 0.0f);
  int previous = -1;
  int delivered = 0;
  while (previous < kPushed - 1) {
    output.FillCallbackBuffer(out.data(), out.size());
    if (out[0] <= 0.0f) {
      std::this_thread::sleep_for(std::chrono::microseconds(50));
      continue;
    }
    const int index = static_cast<int>(out[0]) - 1;
    // Never duplicated or reordered; gaps only where the cap dropped
    // stale blocks.
    EXPECT_GT(index, previous);
    for (int frame = 0; frame < kBlockSize; frame++) {
      ASSERT_FLOAT_EQ(out[frame * kNumChannels + 1],
                      out[frame * kNumChannels] + 0.5f)
          << "torn block " << index << " frame " << frame;
    }
    previous = index;
    delivered++;
  }

  producer.join();
  EXPECT_GT(delivered, kPushed / 2);
}

TEST(AudioOutputRingTest, ResetDropsQueuedAudio) {
  audio::AudioOutput output;

  for (int i = 0; i < 4; i++) {
    AudioBuffer block = MakeBlock(i);
    EXPECT_TRUE(output.PushAudioBuffer(block).ok());
  }

  output.Reset();

  std::vector<float> out(kBlockSize * kNumChannels, -1.0f);
  output.FillCallbackBuffer(out.data(), out.size());
  EXPECT_FLOAT_EQ(out[0], 0.0f);
  EXPECT_EQ(output.GetUnderrunCount(), 0u);
}

}  // namespace soir
