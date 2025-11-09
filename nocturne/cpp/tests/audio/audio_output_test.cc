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

  auto init_status = output.Init(48000, 2, 512);
  EXPECT_TRUE(init_status.ok());

  auto start_status = output.Start();
  EXPECT_TRUE(start_status.ok());

  auto stop_status = output.Stop();
  EXPECT_TRUE(stop_status.ok());
}

}  // namespace soir
