# █▀ █▀█ █ █▀█
# ▄█ █▄█ █ █▀▄ @ https://soir.dev

@live()
def setup():
    bpm.set(120)

    ctrls.mk_lfo('x1', 0.25, low=0.1, high=0.6)

    tracks.setup({
        'lead': tracks.mk_sampler(fxs={
            'chorus': fx.mk_chorus(time=ctrl('x1')),
            'reverb': fx.mk_reverb(),
        })
    })

@loop(beats=4)
def beat():
    log('beat')
