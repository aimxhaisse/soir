import neon

n = neon

n.bpm.set(120)

n.setup_tracks([
    n.mk_track("mono_sampler", 1, muted=False, volume=100),
])

s = n.sampler.Sampler('passage')

# log(str(get_samples('passage')))

@neon.loop(track=1, beats=4)
def kick():
    s.play('fx_ambience_3')
    n.sleep(4)
