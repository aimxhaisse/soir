"""
???+ info

    The sampler module can be used on loops running on tracks with
    instrument type set to `sampler`.

The **sampler** module provides a way to load samples and play them
inside loops in an intuitive way. Once instantiated, a `Sampler` can
be used to play samples from the selected pack given their name. If no
exact match of the sample name is found, the first matching sample is
selected. The cost of creating and using a sampler is cheap so it is
fine to have a lot of instances at once.

# Cookbook

## Play samples

``` python
s = sampler.new('808')

@loop
def kick(beats=4):
  for i in range(4):
    s.play('kick')
    sleep(1)
```

## List available packs

``` python
packs = sampler.packs()
```

## List samples in a pack

``` python
samples = sampler.samples('808')
```

# Reference
"""

from dataclasses import dataclass

import json

from bindings import (
    midi_sysex_sample_play_,
    midi_sysex_sample_stop_,
    get_packs_,
    get_samples_,
    schedule_,
)
from soir._internals import (
    assert_in_loop,
    current_loop,
)
from soir.ctrls import (
    Control,
)
from soir._helpers import (
    serialize_parameters,
)


def new(pack_name: str) -> "Sampler":
    """Creates a new sampler with samples from the designated pack.

    Args:
        pack_name: The name of the sample pack to use.
    """
    return Sampler(pack_name)


class Sampler:

    def __init__(self, pack_name: str):
        """Creates a new sampler with samples from the designated pack.

        ???+ note

            You can create a Sampler via the shortcut `n.sampler.new()`.

        Args:
            pack_name: The name of the sample pack to use.
        """
        self.pack_name_ = pack_name

    def play(
        self,
        name: str = '',
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
    ):
        """Plays a sample by its given name. If there is no exact
        match, attempts to find one that contains the name (for
        example, 'kick' will match 'hard-kick'). If the selected
        sample is already being played, enqueues a new one, allowing
        to play simultaneously multiple times the same sample.

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
        assert_in_loop()

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

        track = current_loop().track

        schedule_(
            current_loop().current_offset,
            lambda: midi_sysex_sample_play_(track, serialize_parameters(params)),
        )

    def stop(self, name: str):
        """Stops playing the sample. If there is no exact match,
        attempts to find one that contains the name (for example,
        'kick' will match 'hard-kick'). If the same sample is
        currently played multiple times, the latest one is selected to
        stop (LIFO).

        Args:
            name: The name of the sample.
        """
        assert_in_loop()

        params = {
            "pack": self.pack_name_,
            "name": name,
        }

        track = current_loop().track

        schedule_(
            current_loop().current_offset,
            lambda: midi_sysex_sample_stop_(track, json.dumps(params)),
        )


@dataclass
class Sample:
    """
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

    Returns:
        The list of loaded sample packs.
    """
    return get_packs_()


def samples(pack_name: str) -> list[Sample]:
    """Returns the list of samples available in the given pack.

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
    def __init__(self, sp: sampler.Sampler):
        """Initialize the Kit with a sampler instance.

        Args:
            sp (sampler.Sampler): The sampler instance to use for playing samples.
        """
        self.sp = sp
        self.duration = current_loop().beats
        self.kit = dict()
        self.patterns = dict()

    def set(self, char: str, mkplay: callable) -> None:
        """Set a character to a sample play function.

        Args:
            char (str): The character that will trigger the sample.
            mkplay (callable): A function that returns a dictionary of sample parameters.
        """
        self.kit[char] = mkplay

    def seq(self, flavor: str, sequences: list[str]) -> None:
        """Define a sequence of samples for a given flavor.

        Args:
            flavor (str): The name of the sequence flavor.
            sequences (list[str]): A list of strings, each representing a sequence of characters.
        Raises:
            ValueError: If the sequences are not of the same length.
        """
        steps = {len(x) for x in sequences}
        if len(steps) != 1:
            raise ValueError('DrumKit sequences must be of the same duration')
        self.patterns[flavor] = sequences

    def play(self, flavor: str) -> None:
        """Play a sequence of samples defined by the flavor.

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
                    sp.play(**what())
            sleep(dur)
