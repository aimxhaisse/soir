@live()
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_sampler(1),
        tracks.mk_midi(2, midi_device=0, audio_device=1),
    ])

    """
    controls.setup({
        'ctrl-0x1': controls.mk(sin(1)),
        'ctrl-0x2': controls.mk(controls.midi_in('x', 1)),
    })
    """
    
s = sampler.new('mxs-beats')

kit = {
    'k': 'bd-808-a-03',
    's': 'sd-909-a-10',
    'h': 'ch-606-mod-16',
}

@loop(track=1, beats=4)
def beats():
    patterns = [
        'k...k...k...k...',
        '....s.......s...',
    ]

    for i in range(16):
        for p in patterns:
            char = p[i]
            if kit.get(char):
                s.play(kit[char])
        sleep(0.25)

@loop(track=2, beats=4)
def digitone():
    for i in range(8):
        with midi.use_chan(1):
            midi.note(60, duration=0.4)
            sleep(0.5)

@loop(track=2, beats=4)
def digitwo():
    for i in range(8):
        with midi.use_chan(1):
            midi.note(63, duration=0.4)
            sleep(0.25)

@loop(track=2, beats=4)
def digithree():
    for i in range(8):
        with midi.use_chan(1):
            midi.note(65, duration=0.4)
            sleep(0.5)
