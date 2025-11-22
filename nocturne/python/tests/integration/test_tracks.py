"""Integration tests for tracks setup and management (tracks module)."""

from .base import SoirIntegrationTestCase


class TestTracksSetup(SoirIntegrationTestCase):
    """Test tracks.setup() for creating and managing tracks."""

    def test_setup_tracks_empty(self) -> None:
        """Test that empty track setup returns empty dict."""
        self.engine.push_code(
            """
tracks.setup({})
log(str(tracks.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("{}"))

    def test_setup_tracks_one(self) -> None:
        """Test creating a single sampler track."""
        self.engine.push_code(
            """
tracks.setup({
  'sp': tracks.mk('sampler')
})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_tracks_three(self) -> None:
        """Test creating three sampler tracks."""
        self.engine.push_code(
            """
tracks.setup({
  'sp1': tracks.mk('sampler'),
  'sp2': tracks.mk('sampler'),
  'sp3': tracks.mk('sampler')
})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp1"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp1, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

        self.assertTrue(self.engine.wait_for_notification("sp2"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp2, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

        self.assertTrue(self.engine.wait_for_notification("sp3"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp3, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_tracks_three_remove_one(self) -> None:
        """Test removing tracks by setting up with fewer tracks."""
        self.engine.push_code(
            """
tracks.setup({
  'sp1': tracks.mk('sampler'),
  'sp2': tracks.mk('sampler'),
  'sp3': tracks.mk('sampler'),
})

"""
        )

        self.engine.push_code(
            """
tracks.setup({
  'sp1': tracks.mk('sampler'),
})

layout = tracks.layout()

log(str(len(layout)))
log(str(layout['sp1']))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("1"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp1, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_tracks_one_then_two(self) -> None:
        """Test adding a second track to existing setup."""
        self.engine.push_code(
            """
tracks.setup({
  'sp1': tracks.mk('sampler'),
})

"""
        )

        self.engine.push_code(
            """
tracks.setup({
  'sp2': tracks.mk('sampler'),
  'sp1': tracks.mk('sampler'),
})

layout = tracks.layout()

log(str(len(layout)))
log(str(layout['sp1']))
log(str(layout['sp2']))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("2"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp1, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp2, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_tracks_one_twice(self) -> None:
        """Test idempotency - setting up same track multiple times."""
        self.engine.push_code(
            """
tracks.setup({
  'sp2': tracks.mk('sampler'),
})

"""
        )

        self.engine.push_code(
            """
tracks.setup({
  'sp2': tracks.mk('sampler'),
})

tracks.setup({
  'sp2': tracks.mk('sampler'),
})

layout = tracks.layout()

log(str(len(layout)))
log(str(layout['sp2']))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("1"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp2, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_fx_one(self) -> None:
        """Test adding a single effect (chorus) to a track."""
        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=['chorus'])"
            )
        )

    def test_setup_fx_one_remove(self) -> None:
        """Test adding and then removing a chorus effect."""
        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=['chorus'])"
            )
        )

        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_fx_multiple_remove_multiple(self) -> None:
        """Test adding multiple effects (chorus + reverb) and removing them."""
        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus(), 'rev': fx.mk_reverb()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=['chorus', 'reverb'])"
            )
        )

        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=[])"
            )
        )

    def test_setup_fx_reorder(self) -> None:
        """Test that effect chain order can be changed."""
        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={'chr': fx.mk_chorus(), 'rev': fx.mk_reverb()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=['chorus', 'reverb'])"
            )
        )

        self.engine.push_code(
            """
tracks.setup({'sp': tracks.mk('sampler', fxs={'rev': fx.mk_reverb(), 'chr': fx.mk_chorus()})})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, "
                "muted=False, volume=1.0, pan=0.0, fxs=['reverb', 'chorus'])"
            )
        )

    def test_setup_track_volume(self) -> None:
        """Test track volume with control reference and constant value."""
        self.engine.push_code(
            """
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', volume=ctrl('c1'))})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, muted=False, "
                "volume=[c1=0.5], pan=0.0, fxs=[])"
            )
        )

        self.engine.push_code(
            """
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', volume=0.3)})

for name, track in tracks.layout().items():
  log(str(track))
"""
        )

        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, muted=False, "
                "volume=0.30000001192092896, pan=0.0, fxs=[])"
            )
        )

    def test_setup_track_pan(self) -> None:
        """Test track pan with control reference and constant value."""
        self.engine.push_code(
            """
ctrls.mk_val("c4", 0.5)
tracks.setup({'sp': tracks.mk('sampler', pan=ctrl('c4'))})

for name, track in tracks.layout().items():
  log(str(name))
  log(str(track))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("sp"))
        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, muted=False, "
                "volume=1.0, pan=[c4=0.5], fxs=[])"
            )
        )

        self.engine.push_code(
            """
ctrls.mk_val("c1", 0.5)
tracks.setup({'sp': tracks.mk('sampler', pan=0.3)})

for name, track in tracks.layout().items():
  log(str(track))
"""
        )

        self.assertTrue(
            self.engine.wait_for_notification(
                "Track(name=sp, instrument=sampler, muted=False, "
                "volume=1.0, pan=0.30000001192092896, fxs=[])"
            )
        )
