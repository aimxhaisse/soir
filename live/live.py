@live()
def setup():
    bpm.set(130)

    tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
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
        'k.k.k.k.k.k.k.k.',
    ]

    for i in range(16):
        for p in patterns:
            char = p[i]
            if kit.get(char):
                s.play(kit[char])
        sleep(0.25)
