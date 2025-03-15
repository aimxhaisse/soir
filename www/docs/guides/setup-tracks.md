# How to setup tracks

In this guide we show how to setup tracks.

## Overview

Tracks can be configured via the `tracks.setup` call as follows:

```python
@live()
def setup():
    tracks.setup({
      'track-1': tracks.mk_sampler(volume=0.5, pan=0.35, fx=[]),
      'track-2': tracks.mk_sampler(),
      'track-3': tracks.mk_sampler(),
    })
```

## Tracks

Each track has a unique name and an instrument which can be selected
via dedicated function calls following the `tracks.mk_` nomenclature.
