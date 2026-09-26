"""Integration tests for the midi module (note and control change events)."""

from typing import Any, ClassVar

from .base import SoirSessionTestCase


class TestMidiCc(SoirSessionTestCase):
    """CC events scheduled via midi.cc() must reach the track's instrument."""

    config_overrides: ClassVar[dict[str, Any]] = {"initial_bpm": 600}

    def test_cc_events_reach_track(self) -> None:
        """midi.cc() must schedule CC events on the loop's track without error.

        The sampler ignores CC events (it only consumes sysex), so this
        exercises the full path (schedule -> binding -> engine queue ->
        track Render) as a delivery smoke test.
        """
        code = """
tracks.setup({'sampler_track': tracks.mk_sampler()})

i = 0

@loop('sampler_track', beats=1)
def cc_events():
    global i
    with midi.use_chan(1):
        midi.cc(7, 100)
    midi.cc(74, 40, chan=2)
    log('cc-iter-' + str(i))
    i += 1
"""
        self.engine.push_code(code)

        for i in range(5):
            self.assertTrue(self.engine.wait_for_notification(f"cc-iter-{i}"))
