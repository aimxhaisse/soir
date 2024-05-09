"""Samples.
"""

class Sampler:
    """Neon's Sampling Machine.
    """

    def __init__(self, pack: str):
        """Creates a new sampler.
        
        Args:
            pack: The sample pack to use.
        """
        pass

    def play(self, name: str):
        """Play a sample.
        
        Args:
            name: The name of the sample.
        """
        pass

    def stop(self, name: str):
        """Stop playing a sample.
        
        Args:
            name: The name of the sample.
        """
        pass


def sample_load(name: str):
    """Load a sample pack on the current track.
    """
    global current_loop_

    if not current_loop_:
        raise NotInLiveLoopException()

    track = current_loop_.track

    params = {
        'name': name,
    }

    schedule_(current_loop_.current_offset, lambda: midi_sysex_sample_load_(track, json.dumps(params)))


def sample_stop(name: str):
    """Stop playing a sample on the current track.
    """
    global current_loop_

    if not current_loop_:
        raise NotInLiveLoopException()

    track = current_loop_.track

    params = {
        'name': name,
    }

    schedule_(current_loop_.current_offset, lambda: midi_sysex_sample_stop_(track, json.dumps(params)))


def sample_play(name: str):
    """Play a sample on the current track.
    """
    global current_loop_

    if not current_loop_:
        raise NotInLiveLoopException()

    track = current_loop_.track

    params = {
        'name': name,
    }

    schedule_(current_loop_.current_offset, lambda: midi_sysex_sample_play_(track, json.dumps(params)))


def get_samples(pack: str) -> list[str]:
    """Get the list of samples.
    """
    return get_samples_from_pack_(pack
)
