# Quick Start

## Installation
       
```
curl -sL https://soir.dev/install.sh | bash
```

For now, only Mac OS with M processing are supported.
            
## Your First Session

Let's start by installing the `hazardous` sample pack:

```sh
soir sample-install-pack hazardous
```

Now we can create a new soir session and run it:
    
```sh
soir new-session session-0x01
soir run-session session-0x01
```

Then in another terminal, open the `session-0x01/live.py` file and paste the following:

```python
@live()
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_sampler(1),
    ])

sp = sampler.new('hazardous')

@loop(beats=8)
def hazardous():
    sp.play('synth-filter2', pan=rnd.between(-1.0, 1.0))
```
