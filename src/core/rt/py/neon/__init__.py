"""*This is your last chance. After this, there is no turning back.*

???+ info

    The **neon** module is the execution context in which music code
    is written and scheduled: it does not need to be referenced and
    all the facilities are accessible directly.

The **neon** module is the base for the Neon environment and is the
context in which you will write and schedule your music code. It
contains a small set of facilities detailed in this as well as more
specialized modules which can be accessed by their names.

# Cookbook

## Minimalistic kick/snare track

``` python
# Set the global BPM.
bpm.set(120)

# Define 1 track with a sampler as an instrument.
tracks.setup([
    tracks.mk_sampler(1, muted=False, volume=100),
    tracks.mk_midi(2, muted=False, volume=100),
])

# Create a sampler instance using the sample pack 'passage'.
s = sampler.new('passage')

# Define a 4 beats loop that plays a kick sample every beat on track 1.
@loop(beats=4, track=1)
def kick():
    for i in range(4):
        s.play('kick')
        sleep(1)

# Define a 2 beats loop that plays a snare sample every 2 beats on track 1.
@loop(beats=2, track=1)
def snare():
    for i in range(2):
        s.play('snare')
        sleep(2)
```

# Reference


"""

import neon.errors
import neon.internals


def loop(beats: int=4, track: int=1, align: bool=True) -> callable:
    """Decorator to create a loop that is rescheduled every given number of beats.

    The concept of a loop is similar to [Sonic
    Pi](https://sonic-pi.net/)'s live loops. Code within a loop is
    executed using temporal recursion, and can updated in real-time:
    the next run of the loop will execute the updated version. This
    provides a way to incrementally build audio performances.  by
    editing code.  Few rules need to be followed for code in
    live-loops: it should not be blocking as it would block the main
    thread. For this reason, blocking facilities such are sleep are
    provided by the engine.

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
