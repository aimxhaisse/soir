# Quick Start

## Installation

Run this command to install soir on your system:

```
curl -sL https://soir.dev/install.sh | bash
```

Then add the following to your shell configuration:

```
export SOIR_DIR="$HOME/.soir"
export PATH="${SOIR_DIR}/bin:$PATH"
```

For now, only Mac OS with M processors are supported.
            
## Your First Session

Let's start by installing the `hazardous` sample pack:

```sh
soir sample-install-pack hazardous
```

Now we can create a new soir session and run it:
    
```sh
soir session-new session-0x01
soir session-run session-0x01
```

Then in another terminal, open the `session-0x01/live.py` file and paste the following:

```python
@live()
def setup():
    bpm.set(120)

    tracks.setup({
        'exp': tracks.mk_sampler(),
    })

sp = sampler.new('hazardous')

@loop(track='exp', beats=8)
def hazardous():
    sp.play('synth-filter2', pan=rnd.between(-1.0, 1.0))
```
