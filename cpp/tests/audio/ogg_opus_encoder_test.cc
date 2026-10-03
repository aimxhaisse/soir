#include "audio/ogg_opus_encoder.hh"

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

namespace soir {

TEST(OggOpusEncoderTest, FramesProduceOggPages) {
  audio::OggOpusEncoder encoder;
  auto status = encoder.Init(48000, 2, 128000);
  ASSERT_TRUE(status.ok());

  std::vector<float> frame(960 * 2);
  for (int i = 0; i < 960; i++) {
    float v = 0.3f * sinf(2.0 * 3.14159265f * 440.0f * i / 48000.0f);
    frame[2 * i] = v;
    frame[2 * i + 1] = v;
  }

  // A 20 ms frame encodes to well under an Ogg page, so pages only
  // come out after a few frames.
  size_t total_bytes = 0;
  for (int i = 0; i < 50; i++) {
    std::vector<uint8_t> out;
    auto s = encoder.Encode(frame.data(), 960, &out);
    ASSERT_TRUE(s.ok()) << s;
    total_bytes += out.size();
  }
  EXPECT_GT(total_bytes, 0u);
}

}  // namespace soir
