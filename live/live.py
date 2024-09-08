bpm.set(110)

@live()
def setup():
    log('setup 2')
    tracks.setup([
        tracks.mk("mono_sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('passage')

@loop(track=1, beats=16)
def beats():
    log('beats KO')
    s.play('morphbot_17')
