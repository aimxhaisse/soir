#include "audio/audio_output.hh"

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

namespace soir {

TEST(AudioOutputTest, Initialization) {
  audio::AudioOutput output;
  auto status = output.Init(48000, 2, 512);
  EXPECT_TRUE(status.ok());
}

TEST(AudioOutputTest, StartStop) {
  audio::AudioOutput output;
  output.Init(48000, 2, 512);

  auto start_status = output.Start();
  EXPECT_TRUE(start_status.ok());

  auto stop_status = output.Stop();
  EXPECT_TRUE(stop_status.ok());
}

TEST(AudioOutputTest, CallbackExecution) {
  audio::AudioOutput output;
  output.Init(48000, 2, 512);

  bool callback_called = false;
  output.SetCallback([&callback_called](float* output, int frame_count) {
    callback_called = true;
    // Fill with silence
    for (int i = 0; i < frame_count * 2; ++i) {
      output[i] = 0.0f;
    }
  });

  output.Start();
  // Give it a moment to call the callback
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  output.Stop();

  EXPECT_TRUE(callback_called);
}

}  // namespace soir
