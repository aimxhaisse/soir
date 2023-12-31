# This code is embedded in Midi at compile time.
#
# It is executed once at startup, before accepting code from Matin
# sessions. Facilities here are for live coding, heavy code can be
# later re-written in c++ if we need to.

from __live import (
    __set_bpm,
    __get_bpm,
    __log
)


def log(message: str) -> None:
    """Log a message to the console.
    """
    __log(message)

    
def set_bpm(bpm: float) -> float:
    """Set the BPM of the current session.
    """
    return __set_bpm(bpm)


def get_bpm() -> float:
    """Get the BPM of the current session.
    """
    return __get_bpm()

