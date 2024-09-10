bpm.set(110)

@live()
def setup():
    log('setup 2')
    tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('passage')

@loop(track=1, beats=8)
def beats():
    log('beats')
    s.play('morphbot_17')
