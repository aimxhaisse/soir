"""
The **ctrls** module contains facilities to control settings of soir
in real-time.

# Cookbook

```python
@live
def setup():
    # Weird approach
    ctrls.matrix['x0'] = ctrls.mk_lfo(rate=0.5, min=0.0, max=1.0, duration=10)
    ctrls.matrix['x1'] = ctrls.mul(ctrls['matrix']['x0'], 0.5)

    ctrls.setup({
        '[x1]': ctrls.mk_lfo(rate=0.5),
        '[x2]': ctrls.mk_val(value=0.1, glide=4),
        '[x3]': ctrls.mk_rnd(min=0.0, max=0.4, glide=4),
        '[x4]': ctrls.mk_path([-1.0, 0.3, 0.6, -0.4]),
        '[x5]': ctrls.mk_expr('([x1] * [x2]) / 2.0f'),
    })

    tracks.setup({
        'bass': tracks.mk_sampler(fxs={
            'reverb': fx.mk_reverb(mix=ctrls.mk_val(0.2, glide=4), room=ctrls.mk_lfo(rate=0.1),
        }),

        'melody': tracks.mk_sampler()
    })


@loop('bass')
def bass():
    with midi.use_chan(1):
        midi.note_on('[x1]')
```
"""

import soir._ctrls
