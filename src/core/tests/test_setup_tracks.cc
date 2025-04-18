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
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
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
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));

  EXPECT_TRUE(WaitForNotification("sp2"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp2, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));

  EXPECT_TRUE(WaitForNotification("sp3"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp3, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
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
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
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
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp2, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
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
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
}

TEST_F(CoreTestBase, SetupFxOne) {
  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=['chorus'])"));
}

TEST_F(CoreTestBase, SetupFxOneRemove) {
  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=['chorus'])"));

  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
}

TEST_F(CoreTestBase, SetupFxMultipleRemoveMultiple) {
  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus(), 'rev': fx.mk_reverb()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(WaitForNotification(
      "Track(name=sp, instrument=sampler, "
      "muted=False, volume=1.0, pan=0.0, fxs=['chorus', 'reverb'])"));

  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, "
                          "muted=False, volume=1.0, pan=0.0, fxs=[])"));
}

TEST_F(CoreTestBase, SetupFxReorder) {
  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus(), 'rev': fx.mk_reverb()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(WaitForNotification(
      "Track(name=sp, instrument=sampler, "
      "muted=False, volume=1.0, pan=0.0, fxs=['chorus', 'reverb'])"));

  PushCode(R"(
tracks.setup({'sp': tracks.mk('sampler', fxs={'rev': fx.mk_reverb(), 'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(WaitForNotification(
      "Track(name=sp, instrument=sampler, "
      "muted=False, volume=1.0, pan=0.0, fxs=['reverb', 'chorus'])"));
}

TEST_F(CoreTestBase, SetupTrackVolume) {
  PushCode(R"(
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', volume=ctrl('c1'))})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, muted=False, "
                          "volume=[c1=0.5], pan=0.0, fxs=[])"));

  PushCode(R"(
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', volume=0.3)})

for name, track in tracks.layout().items():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, muted=False, "
                          "volume=0.30000001192092896, pan=0.0, fxs=[])"));
}

TEST_F(CoreTestBase, SetupTrackPan) {
  PushCode(R"(
ctrls.mk_val("c4", 0.5)
tracks.setup({'sp': tracks.mk('sampler', pan=ctrl('c4'))})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
  )");

  EXPECT_TRUE(WaitForNotification("sp"));
  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, muted=False, "
                          "volume=1.0, pan=[c4=0.5], fxs=[])"));

  PushCode(R"(
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', pan=0.3)})

for name, track in tracks.layout().items():
  log(str(track))
  )");

  EXPECT_TRUE(
      WaitForNotification("Track(name=sp, instrument=sampler, muted=False, "
                          "volume=1.0, pan=0.30000001192092896, fxs=[])"));
}

}  // namespace test
}  // namespace core
}  // namespace soir
