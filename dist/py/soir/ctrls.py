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

import math
import json

from bindings import (
    schedule_,
    get_bpm_,
    midi_sysex_update_controls_,
    controls_get_frequency_update_,
)

controls_registry_ = {}


# This may need to be adjusted to prevent overloading the RT engine,
# we'll likely need some instrumentation to measure the time it takes
# to compute all controls.
frequency_ = controls_get_frequency_update_()
tick_sec_ = 1 / frequency_


class BaseParameter_:
    """Base class for parameters.
    """
    def get_next_value() -> float:
        raise NotImplementedError()


class LFO_(BaseParameter_):
    """A simple LFO parameter.
    """
    def __init__(self, rate: float, intensity: float):
        self.rate_ = rate
        self.intensity_ = intensity
        self.tick_ = 0
                 
    def get_next_value() -> float:
        value = self.intensity_ * math.sin(self.tick_ * 2 * math.pi * self.rate_)
        self.tick_ += tick_sec_
        return value


class Linear_(BaseParameter_):
    """A linear parameter.
    """
    def __init__(self, start: float, end: float, duration: float):
        self.start_ = start
        self.end_ = end
        self.duration_ = duration
        self.tick_ = 0

    def get_next_value() -> float:
        value = self.start_ + (self.end_ - self.start_) * (self.tick_ / self.duration_)
        self.tick_ += tick_sec_
        return value
    

def update_loop_():
    """Ticker for the controls.

    This needs to be fast as it is the temporal recursion that is
    called 100 times per second. It is responsible for updating the
    controls with fresh values which are sent as MIDI events to the
    controller destination.
    """
    payload = {'knobs': {}, 'frequency': frequency_}

    # We sort by alphabetical order to ensure that dependencies are
    # correctly resolved.
    for name, ctrl in dict(sorted(controls_registry_.items())):
        ctrl.update()
        payload['knobs'][name] = ctrl.get_next_value()

    midi_sysex_update_controls_(json.dumps(payload))
    
    next_at = (1 / frequency_) * get_bpm_() / 60
    schedule_(next_, update_loop)
