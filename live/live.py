bpm.set(100)

@live()
def setup():
    tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('phandora')

@live()
def debug():
    samples = sampler.samples('phandora')
    for sample in samples:
        if 'kick' in sample.name:
            log(sample.name)

@loop(track=1, beats=4)
def main():
    s.play('sd_melodic2')
    s.play('loop_100_fromjapan')
