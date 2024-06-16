bpm.set(110)


tracks.setup([
    tracks.mk("midi_ext", 1, muted=False, volume=100),
    tracks.mk("mono_sampler", 2, muted=False, volume=100),
])


afx = [
    [64, 69, 75],
    [64, 68, 73],
]


pattern = "12123-12123-123-"


@loop(track=1, beats=8)
def arp():
    it = 0
    for base in afx:
        for i in pattern:
            it += 1
            if i == "-":
                sleep(0.25)
                continue
            note = base[int(i) - 1]
            if it % 5 == 0:
                note += 5
            if it % 7 == 0:
                note -= 5
            midi.note_on(note, 100)
            sleep(0.25)
            midi.note_off(note, 100)
