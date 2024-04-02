# N E O N

Neon is a standalone binary for live-coding. Think of it as a DAW
where orchestration is handled from code.

## Guiding Principles

- Single binary for everything
- Simple Python syntax
- Keep your own editor
- Start simple

## Design Patterns

Consistency is key as it will become complex with time, some arbitrary
patterns we are using:

- Threads are handled by the object and not the caller
- Threads implement `Init/Start/Stop` pattern

### Runtime routines

#### Sync outside of loops

Synchronuous routines that take time should not happen within loop, as
they will screw the timing of events. Prefer allowing them in the
global context: they might only delay evaluation of the next loop
update, which will have the loops repeat once more for their duration
but without being a total disaster.

Examples:

- loading a sample directory
- getting the list of samples
- creating a websocket or a Unix thread

#### Async from loops

Conversely, events going from loops should use MIDI as they are async
and return immediately, later on we can add timing information into
them and properly handle eventual interpretation lag.

Examples:

- playing a specific sample via MIDI

## Roadmap

- Consider renaming log to notification system
- ADSR around samples
- Select sample via name through Midi
- IPFS
- Websocket queue


