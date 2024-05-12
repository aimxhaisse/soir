import neon

neon.set_bpm(120)

neon.setup_tracks([
    neon.mk_track("mono_sampler", 1, muted=False, volume=100),
])

s = neon.sampler.Sampler('passage')

# log(str(get_samples('passage')))

@neon.loop(track=1, beats=4)
def kick():
    s.play('fx_ambience_3')
    neon.sleep(4)
