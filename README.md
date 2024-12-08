# S O I R

[![experimental](http://badges.github.io/stability-badges/dist/experimental.svg)](http://github.com/badges/stability-badges)

Soir is an environment for audio live-coding.

```python
bpm.set(110)

@live
def setup():
    tracks.setup([
        tracks.mk("mono_sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('passage')

@loop(track=1, beats=4)
def beats():
    for i in range(4):
        s.play('kick')
        if i  % 2 == 0:
            s.play('snare')
        sleep(1)
```

## Features

- loops
- multi-track
- sampler
- external MIDI device
