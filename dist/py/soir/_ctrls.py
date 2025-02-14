import math
import json

import soir.errors

from bindings import (
    schedule_,
    get_bpm_,
    midi_sysex_update_controls_,
    controls_get_frequency_update_,
)

in_update_loop_ = False
controls_registry_ = {}


# This may need to be adjusted to prevent overloading the RT engine,
# we'll likely need some instrumentation to measure the time it takes
# to compute all controls.
frequency_ = controls_get_frequency_update_()
tick_sec_ = 1 / frequency_


def assert_in_update_loop():
    """Assert that we are in the update loop.
    """
    if not in_update_loop_:
        raise errors.NotInControlUpdateLoopException()


class Control_:
    """Base class for a control.

    Publicly documented in ctrls.Control.
    """
    def __init__(self, name):
        self.name_ = name
        controls_registry_[name] = self

    def __repr__(self) -> str:
        return f'[{self.name_}={self.get()}]'

    def set(self, **params) -> None:
        raise NotImplementedError()

    def get(self) -> float:
        raise NotImplementedError()
        
    def fwd(self) -> float:
        raise NotImplementedError()

    
class LFO_(Control_):
    """A simple LFO parameter.
    """
    def __init__(self, name: str, rate: float, intensity: float):
        super().__init__(name)

        self.rate_ = rate
        self.intensity_ = intensity
        self.tick_ = 0
        self.value_ = 0

    def get(self) -> float:
        return self.value_
                 
    def fwd(self) -> float:
        assert_in_update_loop()

        self.value_ = self.intensity_ * math.sin(self.tick_ * 2 * math.pi * self.rate_)
        self.tick_ += tick_sec_


class Linear_(Control_):
    """A linear parameter.
    """
    def __init__(self, name: str, start: float, end: float, duration: float):
        super().__init__(name)

        self.start_ = start
        self.end_ = end
        self.duration_ = duration
        self.tick_ = 0
        self.value_ = start

    def get(self) -> float:
        return self.value_

    def fwd(self) -> float:
        assert_in_update_loop()

        self.value_ = self.start_ + (self.end_ - self.start_) * (self.tick_ / self.duration_)
        self.tick_ += tick_sec_


def update_loop_():
    """Ticker for the controls.

    This needs to be fast as it is the temporal recursion that is
    called 100 times per second. It is responsible for updating the
    controls with fresh values which are sent as MIDI events to the
    controller destination.
    """
    global in_update_loop_
    in_update_loop_ = True

    payload = {'knobs': {}}

    # We sort by alphabetical order to ensure that dependencies are
    # correctly resolved.
    for name, ctrl in dict(sorted(controls_registry_.items())).items():
        ctrl.fwd()
        payload['knobs'][name] = ctrl.get()

    midi_sysex_update_controls_(json.dumps(payload))

    next_at = (1 / frequency_) * get_bpm_() / 60
    schedule_(next_at, update_loop_)

    in_update_loop_ = False
