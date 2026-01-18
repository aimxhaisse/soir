"""
The sampler module can be used on loops running on tracks with
instrument type set to `sampler`.

@public

The **sampler** module provides a way to load samples and play them
inside loops in an intuitive way. Once instantiated, a `Sampler` can
be used to play samples from the selected pack given their name. If no
exact match of the sample name is found, the first matching sample is
selected. The cost of creating and using a sampler is cheap so it is
fine to have a lot of instances at once.

```python
s = sampler.new('808')

@loop
def kick(beats=4):
  for i in range(4):
    s.play('kick')
    sleep(1)
```
"""

from dataclasses import dataclass
from typing import Any, Callable

import json

from soir._bindings.rt import (
    midi_sysex_sample_play_,
    midi_sysex_sample_stop_,
    get_packs_,
    get_samples_,
    schedule_,
)
from soir.rt._internals import (
    assert_in_loop,
    sleep,
)
from soir.rt.ctrls import (
    Control,
)
from soir.rt.errors import (
    SamplePackNotFoundException,
)
from soir.rt._helpers import (
    serialize_parameters,
)


def new(pack_name: str) -> "Sampler":
    """Creates a new sampler with samples from the designated pack.

    @public

    Args:
        pack_name: The name of the sample pack to use.
    """
    if pack_name not in packs():
        raise SamplePackNotFoundException(
            f"{pack_name} not available, add it to your session"
        )
    return Sampler(pack_name)


class Sampler:
    """A sampler allows playing samples from a given sample pack.

    @public

    Use the shortcut `sampler.new()` to create a `Sampler`.

    """

    def __init__(self, pack_name: str):
        """Creates a new sampler with samples from the designated pack.

        Args:
            pack_name: The name of the sample pack to use.
        """
        self.pack_name_ = pack_name

    def play(
        self,
        name: str = "",
        start: float = 0.0,
        end: float = 1.0,
        pan: float | Control = 0.0,
        attack: float = 0.0,
        decay: float = 0.0,
        sustain: float | None = None,
        level: float = 1.0,
        release: float = 0.0,
        rate: float = 1.0,
        amp: float = 1.0,
    ) -> None:
        """Plays a sample by its given name. If there is no exact
        match, attempts to find one that contains the name (for
        example, 'kick' will match 'hard-kick'). If the selected
        sample is already being played, enqueues a new one, allowing
        to play simultaneously multiple times the same sample.

        @public

        Args:
            name: The name of the sample.
            start: When in the sample to start playing in the [0.0, 1.0] range.
            end: When in the sample to end playing in the [0.0, 1.0] range.
            pan: The panning of the sample in the [-1.0, 1.0] range, or a control.
            attack: The attack time in seconds.
            decay: The decay time in seconds.
            sustain: The sustain time in seconds, infered from the sample duration if None.
            release: The release time in seconds.
            level: The sustain level in the [0.0, 1.0] range.
            rate: The playback rate of the sample.
            amp: The amplitude of the sample.
        """
        loop = assert_in_loop()

        params = {
            "pack": self.pack_name_,
            "name": name,
            "start": start,
            "end": end,
            "pan": pan,
            "attack": attack,
            "decay": decay,
            "level": level,
            "release": release,
            "rate": rate,
            "amp": amp,
        }

        track = loop.track

        schedule_(
            loop.current_offset,
            lambda: midi_sysex_sample_play_(track, serialize_parameters(params)),
        )

    def stop(self, name: str) -> None:
        """Stops playing the sample. If there is no exact match,
        attempts to find one that contains the name (for example,
        'kick' will match 'hard-kick'). If the same sample is
        currently played multiple times, the latest one is selected to
        stop (LIFO).

        @public

        Args:
            name: The name of the sample.
        """
        loop = assert_in_loop()

        params = {
            "pack": self.pack_name_,
            "name": name,
        }

        track = loop.track

        schedule_(
            loop.current_offset,
            lambda: midi_sysex_sample_stop_(track, json.dumps(params)),
        )


@dataclass
class Sample:
    """Represents a sample from a sample pack.

    @public

    A sample is a sound file that can be played by a sampler. It has a
    name, a pack, a path, and a duration. The name is the identifier
    used to play the sample, the pack is the name of the pack the
    sample belongs to, the path is the location of the sample file, and
    the duration is the length of the sample in seconds.

    Attributes:
        name: The name of the sample.
        pack: The name of the pack.
        path: The location of the sample file.
        duration: The length of the sample in seconds.
    """

    name: str
    pack: str
    path: str
    duration: float


def packs() -> list[str]:
    """Returns the list of available sample packs.

    @public

    Returns:
        The list of loaded sample packs.
    """
    return get_packs_()  # type: ignore[no-any-return]


def samples(pack_name: str) -> list[Sample]:
    """Returns the list of samples available in the given pack.

    @public

    Args:
        pack_name: The name of the sample pack.

    Returns:
        The list of samples from the sample pack.
    """
    result = []
    for s in get_samples_(pack_name):
        result.append(Sample(name=s, pack=pack_name, path="", duration=0.0))
    return result


class Kit:
    """A simple kit for playing patterns of samples.

    @public

    This class allows you to define a set of samples (or "plays") that can be
    triggered by characters, and then create sequences of these characters to
    play patterns.

    Example usage:

    ```python
    sp = sampler.new('my-sample-pack')

    kit = sampler.Kit(sp)

    kit.set('k', lambda: {'name': 'bd-808'})
    kit.set('s', lambda: {'name': 'sd-808'})

    kit.seq('basic', [
        'k-------k-------',
        '--------s-------',
    ])

    kit.play('basic')
    ```

    This will play a basic kick and snare pattern using the defined samples.
    """

    def __init__(self, sp: Sampler):
        """Initialize the Kit with a sampler instance.

        @public

        Args:
            sp (sampler.Sampler): The sampler instance to use for playing samples.
        """
        self.sp = sp
        loop = assert_in_loop()
        self.duration: float = loop.beats
        self.kit: dict[str, Callable[[], dict[str, Any]]] = {}
        self.patterns: dict[str, list[str]] = {}

    def set(self, char: str, mkplay: Callable[[], dict[str, Any]]) -> None:
        """Set a character to a sample play function.

        @public

        Args:
            char (str): The character that will trigger the sample.
            mkplay (callable): A function that returns a dictionary of sample parameters.
        """
        self.kit[char] = mkplay

    def seq(self, flavor: str, sequences: list[str]) -> None:
        """Define a sequence of samples for a given flavor.

        @public

        Args:
            flavor (str): The name of the sequence flavor.
            sequences (list[str]): A list of strings, each representing a sequence of characters.
        Raises:
            ValueError: If the sequences are not of the same length.
        """
        steps = {len(x) for x in sequences}
        if len(steps) != 1:
            raise ValueError("DrumKit sequences must be of the same duration")
        self.patterns[flavor] = sequences

    def play(self, flavor: str) -> None:
        """Play a sequence of samples defined by the flavor.

        @public

        Args:
                flavor (str): The name of the sequence flavor to play.
        Raises:
                ValueError: If the flavor does not exist in the patterns.
        """
        pattern = self.patterns.get(flavor)
        if not pattern:
            raise ValueError(f"DrumKit has no flavor named {flavor}")

        steps = len(pattern[0])
        dur = self.duration / steps

        for i in range(steps):
            for p in pattern:
                what = self.kit.get(p[i])
                if what:
                    self.sp.play(**what())
            sleep(dur)
