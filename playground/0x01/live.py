# █▀ █▀█ █ █▀█
# ▄█ █▄█ █ █▀▄ @ https://soir.dev

# Experiments around Signatune.


@live()
def setup():
    bpm.set(120)

    ctrls.mk_lfo("x1", 0.1, low=0.1, high=0.2)
    ctrls.mk_lfo("x2", 0.25, low=0.1, high=0.6)
    ctrls.mk_lfo("x3", 0.02)
    ctrls.mk_lfo("x4", 0.01, low=0.1, high=0.4)
    ctrls.mk_lfo("x5", 0.01, low=0.25, high=1.0)

    tracks.setup(
        {
            "lead": tracks.mk_sampler(
                volume=0.5,
            ),
            "pads": tracks.mk_sampler(
                volume=0.4,
                fxs={"reverb": fx.mk_reverb(time=2.0, dry=0.7, wet=1)},
            ),
        }
    )


sp = sampler.new("mxs-samples")


signatune_1 = [
    {"start": 0 / 16, "dur": 0.75 / 16},
    {"start": 0.5 / 16, "dur": 0.5 / 16},
    {"start": 0.75 / 16, "dur": 0.25 / 16},
    {"start": 0.5 / 16, "dur": 0.5 / 16},
    {"start": 0.5 / 16, "dur": 0.25 / 16},
    {"start": 0.5 / 16, "dur": 0.5 / 16},
    {"start": 0.75 / 16, "dur": 0.25 / 16},
    {"start": 0.75 / 16, "dur": 0.25 / 16},
    {"start": 0.75 / 16, "dur": 0.25 / 16},
]

signatune_2 = [
    {"start": 4 / 16, "dur": 0.75 / 16},
    {"start": 4.5 / 16, "dur": 0.5 / 16},
    {"start": 4.75 / 16, "dur": 0.25 / 16},
    {"start": 4.5 / 16, "dur": 0.5 / 16},
    {"start": 4 / 16, "dur": 0.75 / 16},
    {"start": 4.5 / 16, "dur": 0.25 / 16},
    {"start": 4.75 / 16, "dur": 0.25 / 16},
    {"start": 4.75 / 16, "dur": 0.25 / 16},
    {"start": 4.75 / 16, "dur": 0.25 / 16},
]

signatune_3 = [
    {"start": 8 / 16, "dur": 0.75 / 16},
    {"start": 8.5 / 16, "dur": 0.25 / 16},
    {"start": 8 / 16, "dur": 0.75 / 16},
    {"start": 8.75 / 16, "dur": 0.25 / 16},
    {"start": 8 / 16, "dur": 0.75 / 16},
    {"start": 8.5 / 16, "dur": 0.25 / 16},
    {"start": 8 / 16, "dur": 0.75 / 16},
    {"start": 8.5 / 16, "dur": 0.25 / 16},
]

signatune_4 = [
    {"start": 12 / 16, "dur": 0.75 / 16},
    {"start": 12.5 / 16, "dur": 0.25 / 16},
    {"start": 12 / 16, "dur": 0.75 / 16},
    {"start": 12 / 16, "dur": 0.75 / 16},
    {"start": 12.5 / 16, "dur": 0.25 / 16},
]


@loop(track="lead", beats=16)
def lead():
    for pattern in [signatune_1, signatune_2, signatune_3, signatune_4]:
        slept = 0
        for splay in pattern:
            params = {
                "start": splay["start"],
                "end": splay["start"] + splay["dur"],
                "pan": rnd.between(-0.1, 0.1),
            }

            for rate in [1]:
                params["rate"] = rate
                sp.play("dynasty-freaking-intro-120", **params)

            sleep_for = splay["dur"] * 16
            sleep(sleep_for)
            slept += sleep_for

        sleep(4 - slept)


@loop(track="pads", beats=16)
def pads():
    return
    for pattern in [signatune_1, signatune_2, signatune_3, signatune_4]:
        for i in range(16):
            splay = pattern[0]
            params = {
                "start": splay["start"],
                "end": splay["start"] + splay["dur"],
                "pan": rnd.between(-0.1, 0.1),
            }
            params["rate"] = 2
            sp.play("dynasty-freaking-intro-120", **params)
            sleep(1 / 4)
