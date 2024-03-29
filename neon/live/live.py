set_bpm(120)

setup_tracks([
    mk_track("mono_sampler", 1, muted=False, volume=100),
])

@loop(track=1, beats=4)
def kick():
    for i in range(4):
        sample(30)
        sleep(1)

@loop(track=1, beats=4)
def snare():
    for i in range(2):
        sample(45)
        sample(40)
        sample(50)
        sleep(2)

@loop(track=1, beats=4)
def bass():
    for i in range(5):
        sample(60)
        sleep(0.75)

@loop(track=1, beats=4)
def bass2():
    for i in range(3):
        sleep(0.77)
        sample(64)
