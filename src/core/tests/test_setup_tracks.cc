#include "base.hh"

namespace soir {
namespace core {
namespace test {

TEST_F(CoreTestBase, SetupTracksEmpty) {
  PushCode(R"(
tracks.setup([])
log(str(tracks.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, SetupTracksOne) {
  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 1)
])

for track in tracks.layout():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=1, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksThree) {
  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 1),
  tracks.mk('sampler', 2),
  tracks.mk('sampler', 3),
])

for track in tracks.layout():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=1, "
                          "muted=False, volume=127, pan=64)"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=2, "
                          "muted=False, volume=127, pan=64)"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=3, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksThreeRemoveOne) {
  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 1),
  tracks.mk('sampler', 2),
  tracks.mk('sampler', 3),
])

)");

  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 1),
])

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks[0]))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=1, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksOneThenTwo) {
  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 2),
])

)");

  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 2),
  tracks.mk('sampler', 1),
])

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks[0]))
log(str(tracks[1]))
)");

  EXPECT_TRUE(WaitForNotification("2"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=1, "
                          "muted=False, volume=127, pan=64)"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=2, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksOneTwice) {
  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 2),
])

)");

  PushCode(R"(
tracks.setup([
  tracks.mk('sampler', 2),
])

tracks.setup([
  tracks.mk('sampler', 2),
])

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks[0]))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("Track(instrument=sampler, track=2, "
                          "muted=False, volume=127, pan=64)"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
