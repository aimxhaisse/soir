set_bpm(120)

setup_tracks([
    mk_track("mono_sampler", 1, muted=False, volume=100),
])


def make_doc():
    from mkdocs.__main__ import cli
    cli(['build', '--site-dir', 'site/', '--config-file', 'www/mkdocs.yml'])


make_doc()
    
# log(str(get_samples('passage')))

# Ideal API for samples:
#
# sp = Sampler('passage')
# sp.play('kick')
# sp.stop('kick')
# sp.get()

@loop(track=1, beats=4)
def kick():
    # sample_load('passage')
    for i in range(4):
        log('kick')
        # sample_play('kick_acid')
        sleep(1)
