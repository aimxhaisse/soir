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
