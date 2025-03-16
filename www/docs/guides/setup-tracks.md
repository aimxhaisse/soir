# How to setup tracks

In this guide we show how to setup tracks.

## Overview

Tracks can be configured via the
[tracks.setup](/reference/tracks/#soir.tracks.setup) call as follows:

```python
@live()
def setup():
    tracks.setup({
      'track-1': tracks.mk_sampler(volume=0.5, pan=0.35, fx=[]),
      'track-2': tracks.mk_sampler(),
      'track-3': tracks.mk_sampler(),
    })
```

This call can potentially do heavy operations such as instanciating
new tracks and effects if they don't exist. It is sound to call it
once in a [live](/reference/soir/#soir.live) function, to execute it
upon code updates if the track layout changes.

## Track Settings

All tracks have the following settings which can be bound to dynamic
control parameters or direct values:

- volume
- pan
- muted

Here is an example of two sampler tracks, one with an LFO on the
volume, the other with a direct value:

```python
@live()
def controls():
    ctrls.mk_lfo('[x1]', rate=0.5, intensity=0.75)

@live()
def setup():
    tracks.setup({
      'track-1': tracks.mk_sampler(volume=ctrl('[x0]')),
      'track-2': tracks.mk_sampler(volume=0.65),
    })
```

## Sampler Instrument

A track can be assigned to a sampler instrument via the
[tracks.mk_sampler](/reference/tracks/#soir.tracks.mk_sampler)
function as follows:

```python
@live()
def setup():
    tracks.setup({
      'track-1': tracks.mk_sampler(),
    })
```

## External Instrument
