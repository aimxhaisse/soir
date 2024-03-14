# N E O N

## Architecture

Neon is composed of two parts:

- client
- server

## Patterns

Consistency is key as it will become complex with time, some arbitrary
patterns we are using:

- Threads are handled by the object and not the caller
- Threads implement `Init/Start/Stop` pattern

## Nice to have

This is to be picked if some free-time and no energy to drive big
changes:

- Better failure on config changes
- More unit tests around the Midi code
- Check if it makes sense to use absl Mutexes instead of C++20
- Find a way to document what we export via Pybind
- Loose authentication using gRPC facilities

## Ramblings

### About Python async I/O

Current problems: we can't block in live loops, and this implies at
the beginning of the loop everything is evaluated at once. This
implies you can't for instance call `get_bpm` in the middle of the
live-loop and expect it to return the right value: it will be the
value at the beginning of the loop. Unsure here but maybe using a
Python async I/O approach would solve this. Unclear how pybind/async
I/O would work together.

Syntax of async I/O is annoying, but we could provide helpers to make
it look natural. On the plus side this could open the way to external
interactions.

This [project](https://github.com/DmitryKuk/asynchronizer/tree/master)
needs to be explored to see how it could work. This
[topic](https://stackoverflow.com/questions/71082517/integrate-embedded-python-asyncio-into-boostasio-event-loop)
can help too.
