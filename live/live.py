@live()
def setup():
    bpm.set(130)

    tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('mxs-beats')

@loop(track=1, beats=4)
def main():
    s.play('cy-606-mod-01')
