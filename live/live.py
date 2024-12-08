@live()
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_sampler(1),
    ])


@loop()
def beat():
    log('ok')
