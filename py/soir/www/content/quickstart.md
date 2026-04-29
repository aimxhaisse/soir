# quickstart

## Installation

Soir runs on **Linux only** for now.

We recommend installing it inside a sandboxed environment such as a virtual machine or container. Soir performs real-time audio synthesis and runs Python code with the GIL disabled (`-Xgil=0`), which gives it full access to your system. A VM provides an extra layer of isolation while you experiment.

Soir requires **Python 3.14.2** (free-threaded build). If your distribution packages it, install it first; otherwise use [pyenv](https://github.com/pyenv/pyenv) or [uv](https://docs.astral.sh/uv/):

```bash
# With uv
uv python install 3.14.2t

# With pyenv
pyenv install 3.14.2t
```

Then install Soir from PyPI:

```bash
pip install soir
```

The `soir` command will automatically re-execute under `-Xgil=0` for you.

## Directories

Soir uses two kinds of directories:

| Directory | Purpose |
|---|---|
| **`$SOIR_HOME`** | Global user-data directory. Holds your default `config.json`, installed sample packs, and log files. Defaults to the platform user-data dir (e.g. `~/.local/share/soir`) unless you override it with the `SOIR_HOME` environment variable. |
| **Session directory** | A per-project workspace created with `soir session mk`. Contains your live coding script, local config, samples, and logs. |

A session directory looks like this:

```
my-session/
├── etc/
│   └── config.json      # Engine config (BPM, audio device, sample packs, ...)
├── lib/
│   └── samples/         # Local sample packs
├── var/
│   └── log/             # Session logs
└── live.py              # Your live coding entry point
```

## Your first session

Create a session and start the engine:

```bash
soir session mk my-session
soir session run my-session
```

The TUI will open. Edit `my-session/live.py` in your favourite editor. Every time you save the file, Soir reloads it automatically.

Replace the contents with:

```python
@live()
def setup():
    tracks.setup({"kick": inst.mk_sampler()})

sp = sampler.new("default")

@loop(track="kick", beats=4, align=True)
def beat():
    for i in range(4):
        sp.play("kick")
        sleep(1)
```

Save the file. You should hear a kick drum on every beat.

- `@live()` runs once every time the file is saved. Use it for one-shot setup.
- `@loop()` schedules a function to run repeatedly. `sleep(1)` advances one beat inside the loop.
- `tracks.setup()` creates named instrument tracks.
- `inst.mk_sampler()` gives you a sample player.
- `sampler.new("default")` loads the built-in "default" sample pack.

Press **Ctrl+C** in the TUI to stop the engine.

That's it — you are live-coding sound. Check the [examples](/examples) and [reference](/reference) for more.
