import enum
import math
import json

import soir.errors
import soir._internals

from bindings import (
    schedule_,
    get_bpm_,
    midi_sysex_update_controls_,
    controls_get_frequency_update_,
)

# Incremented at each buffer evaluation, used to remove controls
# that aren't defined in the global scope anymore.
#
# Similar to the approach in internals, duplicated to remove
# entanglement.
eval_id_ = 0


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
    class Scope(enum.Enum):
        """Scope of the control.
        """
        GLOBAL = 0
        FUNC = 1
    
    def __init__(self, name):
        self.name_ = name
        self.tick_ = 0
        self.value_ = 0

        # This is a nice trick, if the control exists, we replace it
        # with the new one, but we keep its tick/value state so that
        # updating it performs a smooth-transition, or a no-op if
        # parameters are the same. This is a two-line to handle all
        # kinds of updates without having to do complex state updates.
        if name in controls_registry_:
            self.tick_ = controls_registry_[name].tick_
            self.value_ = controls_registry_[name].value_

        # We need to keep the func scope here so that we know how to
        # clean up the resource when it's not around anymore.
        func = soir._internals.current_loop() or soir._internals.current_live()
        if func:
            self.scope_ = Control_.Scope.FUNC
            self.scope_func_name_ = func.name
            self.scope_func_eval_at_ = func.eval_at
        else:
            self.scope_ = Control_.Scope.GLOBAL
            self.scope_global_eval_id_ = eval_id_

        controls_registry_[name] = self

    def __repr__(self) -> str:
        return f'[{self.name_}={self.get()}]'

    def get(self) -> float:
        return self.value_

    # This is meant to be implemented by inheriting classes.

    def set(self, **params) -> None:
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

        self.value_ = start

    def fwd(self) -> float:
        assert_in_update_loop()

        self.value_ = self.start_ + (self.end_ - self.start_) * (self.tick_ / self.duration_)
        self.tick_ += tick_sec_


class Val_(Control_):
    """A value parameter.
    """
    def __init__(self, name: str, value: float):
        super().__init__(name)

        self.value_ = value

    def set(self, value) -> None:
        self.value_ = value

    def fwd(self) -> float:
        pass


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


def post_eval():
    """Called after each buffer evaluation.

    Currently used to clean-up buffers that aren't defined anymore.
    """
    global eval_id_

    delete = []

    # This logic is tricky, all cases are unit tested, be sure to
    # cover all cases if updating this code.

    for name, ctrl in controls_registry_.items():
        if ctrl.scope_ == Control_.Scope.GLOBAL:
            if eval_id_ != ctrl.scope_global_eval_id_:
                # Here it means, last time we created this control,
                # it was on a different global eval id, so in a prio
                # eval: this means the control is not defined anymore
                # in the global scope and we can remove it.
                delete.append(name)
                continue
        if ctrl.scope_ == Control_.Scope.FUNC:
            live = soir._internals.get_live(ctrl.scope_func_name)
            loop = soir._internals.get_loop(ctrl.scope_func_name)
            func = live or loop
            if not func or func.eval_at != ctrl.scope_func_eval_at_:
                # Here it means, either the live/loop function was
                # removed or it evalued without re-creating the
                # control which means the control is not there
                # anymore.
                delete.append(name)
                continue

    for d in delete:
        del controls_registry_[d]

    eval_id_ += 1
