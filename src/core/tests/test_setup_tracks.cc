#include "base.hh"

namespace soir {
namespace core {
namespace test {

TEST_F(CoreTestBase, SetupTracksEmpty) {
  PushCode(R"(
tracks.setup({})
log(str(tracks.layout()))
  )");

  EXPECT_TRUE(WaitForNotification("{}"));
}

TEST_F(CoreTestBase, SetupTracksOne) {
  PushCode(R"(
tracks.setup({
  'sp': tracks.mk('sampler')
})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksThree) {
  PushCode(R"(
tracks.setup({
  'sp1': tracks.mk('sampler'),
  'sp2': tracks.mk('sampler'),
  'sp3': tracks.mk('sampler')
})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp1"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp1, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));

  EXPECT_TRUE(WaitForNotification("sp2"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp2, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));

  EXPECT_TRUE(WaitForNotification("sp3"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp3, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksThreeRemoveOne) {
  PushCode(R"(
tracks.setup({
  'sp1': tracks.mk('sampler'),
  'sp2': tracks.mk('sampler'),
  'sp3': tracks.mk('sampler'),
})

)");

  PushCode(R"(
tracks.setup({
  'sp1': tracks.mk('sampler'),
})

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks['sp1']))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp1, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksOneThenTwo) {
  PushCode(R"(
tracks.setup({
  'sp1': tracks.mk('sampler'),
})

)");

  PushCode(R"(
tracks.setup({
  'sp2': tracks.mk('sampler'),
  'sp1': tracks.mk('sampler'),
})

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks['sp1']))
log(str(tracks['sp2']))
)");

  EXPECT_TRUE(WaitForNotification("2"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp1, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp2, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
}

TEST_F(CoreTestBase, SetupTracksOneTwice) {
  PushCode(R"(
tracks.setup({
  'sp2': tracks.mk('sampler'),
})

)");

  PushCode(R"(
tracks.setup({
  'sp2': tracks.mk('sampler'),
})

tracks.setup({
  'sp2': tracks.mk('sampler'),
})

tracks = tracks.layout()

log(str(len(tracks)))
log(str(tracks['sp2']))
)");

  EXPECT_TRUE(WaitForNotification("1"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp2, instrument=sampler, "
                          "muted=False, volume=127, pan=64)"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
