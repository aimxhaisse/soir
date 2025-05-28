#include <thread>

#include "base.hh"

namespace soir {
namespace core {
namespace test {

TEST_F(CoreTestBase, SystemAudioOutDevicesTest) {
  PushCode(R"(
if (len(system.get_audio_out_devices())):
    log("OK")
else:
    log("KO")
  )");

  EXPECT_TRUE(WaitForNotification("OK"));
}

TEST_F(CoreTestBase, SystemAudioInDevicesTest) {
  PushCode(R"(
if (len(system.get_audio_in_devices())):
    log("OK")
else:
    log("KO")
  )");

  EXPECT_TRUE(WaitForNotification("OK"));
}

TEST_F(CoreTestBase, SystemMidiOutDevicesTest) {
  PushCode(R"(
if (len(system.get_midi_out_devices())):
    log("OK")
else:
    log("KO")
  )");

  EXPECT_TRUE(WaitForNotification("OK"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
