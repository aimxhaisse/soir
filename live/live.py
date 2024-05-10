set_bpm(120)

setup_tracks([
    mk_track("mono_sampler", 1, muted=False, volume=100),
])

s = Sampler('passage')

# log(str(get_samples('passage')))

@loop(track=1, beats=4)
def kick():
    s.play('fx_ambience_3')
    sleep(4)
