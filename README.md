# Maethstro L I V E

_matin/midi/soir_

![architecture](assets/live.png)

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

