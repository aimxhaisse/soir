bpm.set(110)

tracks.setup([
    tracks.mk("mono_sampler", 1, muted=False, volume=100),
])

s = sampler.new('passage')

# log(str(sampler.samples('passage')))

@loop(track=1, beats=4)
def beats():
    log('beats')
    for i in range(4):
        s.play('morphbot_17')
        sleep(4)
