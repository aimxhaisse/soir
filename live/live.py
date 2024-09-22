bpm.set(100)

@live()
def setup():
    tracks.setup([
        tracks.mk("sampler", 1, muted=False, volume=100),
    ])

s = sampler.new('mxs-beats')

@live()
def debug():
    samples = sampler.samples('mxs-beats')
    for sample in samples:
        if 'kick' in sample.name:
            log(sample.name)

@loop(track=1, beats=4)
def main():
    pass
