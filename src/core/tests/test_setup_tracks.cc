#include "base.hh"

namespace neon {
namespace core {
namespace test {

TEST_F(CoreTestBase, SetupTracksEmpty) {
  PushCode(R"(
setup_tracks([])
log(str(get_tracks()))
  )");

  EXPECT_TRUE(WaitForNotification("[]"));
}

TEST_F(CoreTestBase, SetupTracksOne) {
  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 1)
])

for track in get_tracks():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=1>"));
}

TEST_F(CoreTestBase, SetupTracksThree) {
  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 1),
  mk_track('mono_sampler', 2),
  mk_track('mono_sampler', 3),
])

for track in get_tracks():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=1>"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=2>"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=3>"));
}

TEST_F(CoreTestBase, SetupTracksThreeRemoveOne) {
  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 1),
  mk_track('mono_sampler', 2),
  mk_track('mono_sampler', 3),
])

)");

  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 1),
])

tracks = get_tracks()

log(str(len(tracks)))
log(str(tracks[0]))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=1>"));
}

TEST_F(CoreTestBase, SetupTracksOneThenTwo) {
  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 2),
])

)");

  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 2),
  mk_track('mono_sampler', 1),
])

tracks = get_tracks()

log(str(len(tracks)))
log(str(tracks[0]))
log(str(tracks[1]))
)");

  EXPECT_TRUE(WaitForNotification("2"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=1>"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=2>"));
}

TEST_F(CoreTestBase, SetupTracksOneTwice) {
  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 2),
])

)");

  PushCode(R"(
setup_tracks([
  mk_track('mono_sampler', 2),
])

setup_tracks([
  mk_track('mono_sampler', 2),
])

tracks = get_tracks()

log(str(len(tracks)))
log(str(tracks[0]))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("<Track instrument='mono_sampler' channel=2>"));
}

}  // namespace test
}  // namespace core
}  // namespace neon
