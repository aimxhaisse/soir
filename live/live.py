@live()
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_midi(1, midi_device=0, audio_device=1),
        tracks.mk_sampler(2),
    ])


notes = [60, 61] #, 63 65]#, 67, 68]
length = len(notes) * 16


def chord(n: int) -> list[int]:
    return [n, n+4, n+7]


@loop(track=1, beats=length)
def motive():
    midi.use_chan(1)
    for base in notes:
        for i in range(32):
            for n in chord(base):
                midi.note(n, duration=0.1)
            sleep(0.5)


@loop(track=1, beats=length)
def pads():
    return
    midi.use_chan(2)
    for base in notes:
        midi.note(base + 12, duration=16)
        sleep(16)


@loop(track=1, beats=length)
def arp():
    return
    midi.use_chan(1)
    for base in notes:
        for i in range(32):
            for n in chord(base):
                midi.note(n + 12, duration=0.1)
                sleep(0.125)
            sleep(0.125)


@loop(track=1, beats=length)
def arp_up():
    return
    midi.use_chan(2)
    for base in notes:
        for i in range(16):
            for n in chord(base):
                midi.note(n + 24, duration=0.25)
                sleep(0.25)
            sleep(0.25)
