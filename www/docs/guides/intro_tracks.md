# Your First Track

The model of Neon is similar to Digital Audio Workstations (a.k.a
DAWs) where a track is identified by an instrument and a channel
number which is used to control the track via MIDI devices. In this
example, we setup two tracks:

- a sampler track that can play samples and be controlled on MIDI channel 0,
- an external MIDI instrument which can be controlled on MIDI channel 1.

``` python
tracks.setup([
    tracks.mk('mono_sampler', 0),
    tracks.mk('midi_ext', 1),
])
```

In the same fashion as with the hello world example, this code can be
updated dynamically, new tracks can be added or removed automatically
from this list. This is how you can for instance progressively move
from an ambient track playing pad samples to something more groovy by
introducing a new track for drums.

The tracks can be setup by accessing facilities from the
[tracks](/reference/tracks) module: this is because Neon is organized
in modules, each targetting a specific area.

As we previously encountered with the [log](/reference/neon/#neon.log)
facilitiy, some keywords globally accessible. Their list is limited.

You can print the current layout of tracks using:
