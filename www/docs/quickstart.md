# Quick Start

Soir can be controlled via the `soir` tool, which provides commands 
to create or load live sessions:

```
$ soir
Available recipes:
    [setup]
    info        # Show environment informations
    check-deps  # Check if dependencies are installed.

    [session]
    new session # Creates a new soir session.
    run session # Runs a soir session.
```

Let's start by creating a new soir session and running it:

```
$ soir new session-0x01
$ soir run session-0x01
```

Now open the `live.py` file from the session-0x01 directory with
your favorite editor:

```
$ emacs session-0x01/live.py
```
