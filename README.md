# S O I R

[![experimental](http://badges.github.io/stability-badges/dist/experimental.svg)](http://github.com/badges/stability-badges)

Soir is an environment for audio live-coding.

```
    Code flows like music
    Algorithms dance through sound
    Python sings alive
```
     
    
```python
bpm.set(110)

@live
def setup():
    tracks.setup({
        'drums': tracks.mk_sampler(muted=False, volume=100),
    })

s = sampler.new('passage')

@loop(track='drums', beats=4)
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
