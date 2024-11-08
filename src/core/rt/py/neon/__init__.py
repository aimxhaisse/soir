"""
Neon is a Python library for live coding music. It provides facilities
to create and manipulate audio tracks, and to interact with external
synthesizers. There are two important concepts in Neon:

- **Live** functions ([`@live`](/reference/neon/#neon.live) decorator)
    that are executed each time the code is changed. They are used to
    setup the environment, and to create tracks and instruments.

- **Loops** functions ([`@loop`](/reference/neon/#neon.loop)
    decorator) that are rescheduled every given number of beats. They
    are used to create patterns and sequences.


``` python
# Live function
@live
def setup():
    bpm.set(120)

# Loop function
@loop(beats=1)
def kick():
    log('beat')
```

Neon's facilities are organized in modules that are accessible from
the global context. For example, to set the BPM, you can use
`bpm.set(120)` without having to explicitly import the `bpm`
module. The available modules are:

- [bpm](/reference/bpm)
- [sampler](/reference/sampler)
- [tracks](/reference/tracks)
- [midi](/reference/midi)

# Cookbook

## Minimalistic Example

``` python
@live
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_sampler(1, muted=False, volume=100),
    ])

s = sampler.new('passage')

@loop(beats=4, track=1)
def kick():
    for i in range(4):
        s.play('kick')
        sleep(1)
```

# Reference


"""

import neon.errors
import neon.internals


def loop(beats: int=4, track: int=1, align: bool=True) -> callable:
    """Decorator to create a loop that is rescheduled every given number of beats.

    The concept of a loop is similar to [Sonic
    Pi](https://sonic-pi.net/)'s live loops. Code within a loop is
    executed using temporal recursion, and can be updated in
    real-time: the next run of the loop will execute the updated
    version. This provides a way to incrementally build audio
    performances by editing code. Loops should not be blocking as it
    would freeze the main thread. For this reason, blocking facilities
    such are [`sleep`](/reference/neon/#neon.sleep) are provided by the
    engine.

    ``` python
    @loop
    def my_loop(beats=4, track=1):
      log("Hello World")
    ```

    Args:
        beats: The duration of the loop in beats.
        track: The track to use.
        align: Whether to align the loop on its next beat sequence.

    Returns:
        A decorator registering and scheduling the function in a loop.
    """
    return neon.internals.loop(beats, track, align)


def live() -> callable:
    """Decorator to create a live function that is executed each time the code is changed.

    ``` python
    @live
    def setup:
      tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
      ])
    ```

    Returns:
        A decorator registering and executing the live function.
    """
    return neon.internals.live()


def log(message: str) -> None:
    """Log a message to the console.

    This function is used to log messages to the console. It is useful
    for debugging purposes.
    
    ``` python
    log(f'We are at beat {bpm.beat()}')
    ```

    Args:
        message: The message to log.
    """
    return neon.internals.log(message)


def sleep(beats: float) -> None:
    """Sleep for the given duration in beats in the current loop.

    In this example, we define a 4-beats loop that plays a kick sample
    every beats, and sleeps for 1 beat between each sample.
        
    ``` python
    @loop
    def my_loop(beats=4, track=1):
       for i in range(4):
          s.play('kick')
          sleep(1)
    ```

    Args:
        beats: The duration to sleep in beats.

    Raises:
        errors.NotInLiveLoopException: If we are not in a loop.
    """
    return neon.internals.sleep(beats)


import neon.bpm as bpm
import neon.sampler as sampler
import neon.tracks as tracks
import neon.midi as midi

# Here lies the live code.
