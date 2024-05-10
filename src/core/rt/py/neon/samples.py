"""*Samples, lots of samples.*

The **sample** module provides a way to load samples and play them
inside loops in an intuitive way. Once instantiated, a sampler can be
used to play samples from the selected pack given their name. If no
exact match of the sample name is found, the first matching sample is
selected. The cost of creating and using a sampler is cheap so it is
fine to have a lot of instances at once.

``` python
s = Sampler('808')

@loop
def kick(beats=4):
  for i in range(4):
    s.play('kick')
    sleep(1)
```

"""


class Sampler:

    def __init__(
            self,
            pack_name: str
    ):
        """Creates a new sampler with samples from the designated pack.

        Args:
            pack_name: The name of the sample pack to use.
        """
        self.pack_name_ = pack_name

    def play(
            self,
            name: str,
            loop: bool = False,
            length: float | None = None,
            start: float = 0.0,
            rev: bool = False,
    ):
        """Plays a sample by its given name. If there is no exact
        match, attempts to find one that contains the name (for
        example, 'kick' will match 'hard-kick'). If the selected
        sample is already being played, enqueues a new one, allowing
        to play simultaneously multiple times the same sample.

        Args:
            name: The name of the sample.
            loop: Whether to loop the sample or not.
                If the sample is looping, it will play forever unless a
                length is specified, or it is explictly stopped.
            length: Duration in seconds.
                If None, the sample is played for its entired duration.
            start: When to start playing the sample in seconds.
            rev: Whether to reverse the sample or not.
        """
        global current_loop_

        if not current_loop_:
            raise NotInLiveLoopException()

        params = {
            'pack': self.pack_name_,
            'name': name,
            'loop': loop,
            'length': length,
            'start': start,
            'rev': rev,
        }

        schedule_(
            current_loop_.current_offset,
            lambda: midi_sysex_sample_play_(track, json.dumps(params))
        )


    def stop(
            self,
            name: str
    ):
        """Stops playing the sample. If there is no exact match,
        attempts to find one that contains the name (for example,
        'kick' will match 'hard-kick'). If the same sample is
        currently played multiple times, the oldest one is selected to
        stop (FIFO).

        Args:
            name: The name of the sample.
        """
        global current_loop_

        if not current_loop_:
            raise NotInLiveLoopException()

        params = {
            'pack': self.pack_name_,
            'name': name,
        }

        schedule_(
            current_loop_.current_offset,
            lambda: midi_sysex_sample_stop_(track, json.dumps(params)))
