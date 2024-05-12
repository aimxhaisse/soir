bpm.set(120)

tracks.setup([
    tracks.mk("mono_sampler", 1, muted=False, volume=100),
])

s = sampler.Sampler('passage')

@loop(track=1, beats=4)
def kick():
    for i in range(4):
        s.play('fx_ambience_3')
        sleep(1)
