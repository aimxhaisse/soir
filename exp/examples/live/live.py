@live()
def setup():
    bpm.set(120)

    tracks.setup([
        tracks.mk_sampler(1),
    ])


s = sampler.new('hexcells')
    
@loop()
def beat(beats=1):
    log('ok')
    s.play('hex-tik-5', start=0.5, end=0.0)
    sleep(0.5)
    s.play('hex-tik-5', start=0.0, end=0.5)
