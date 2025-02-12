# Architecture

Soir is composed of two main parts:

- The RT engine which schedules Python loops,
- The DSP engine which continuously produces blocks of sounds.

As a general rule, only the RT engine can execute Python code: its
main loop takes the GIL lock, then executes callback functions or code
updates which in turn call Python code.
    
The RT engine instruments how the DSP engine should produce sounds,
and to do so it has two main ways of doing it.

## C++ Bindings (blocking)

The Python code can directly call blocking C++ bindings that target
the DSP engine. Such bindings can sometimes be slow, such as setting
up a new track or adding an effect, and as a result are meant to be
wrapped in @live decorators which are executed upon code change
(rarely).
    
## MIDI Events (async)

The Python code can also schedule MIDI events which contains
timing-sensitive actions such as, starting to play a sample, mute a
track. Whenever they are emitted, time is kept to try to interpret
them as close as possible to where they should fit in the DSP blocks.
This is the recommended way to instrument DSP code from within `@loop`
decorated functions.

MIDI was chosen here but it may be a better approach in the future to
switch to OSC as it can carry timing information itself, and also has
a more flexible way to provide parameters.

## Time Handling

On the DSP side, the time is represented by an absolute time in
microseconds and a `SampleTick` (the number of samples produced so
far). On the RT side, the time is represented by a fractional beat
which is not necessarily linear as it can be modulated by the BPM
setting.

Whenever a MIDI event is scheduled, the RT engine can convert the
current beat offset in the loop to an exact `SampleTick` for the DSP
code to properly trigger the event. Because the RT code can take time
to execute and propagate its events, it can be slightly late and
jitter around the exact scheduling. For this reason, the DSP thread
runs in the past and its clock is offset by a few milliseconds
(`kBlockProcessingDelay` blocks, around ~70ms). This lets time for
MIDI events to flow through the DSP engine and arrive on time.
