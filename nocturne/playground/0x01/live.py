# █▀ █▀█ █ █▀█
# ▄█ █▄█ █ █▀▄ @ https://soir.dev

# Experiments around Signatune.


MODE = "live"


@live()
def setup():
    bpm.set(120)

    ctrls.mk_lfo("x1", 0.1, low=0.1, high=0.2)
    ctrls.mk_lfo("x2", 0.25, low=0.1, high=0.6)
    ctrls.mk_lfo("x3", 0.02)
    ctrls.mk_lfo("x4", 0.01, low=0.1, high=0.4)
    ctrls.mk_lfo("x5", 0.1, low=0.3, high=0.7)

    tracks.setup(
        {
            "lead": tracks.mk_sampler(
                volume=0.2,
                fxs={
                    "lpf": fx.mk_lpf(cutoff=ctrl("x5")),
                    "chorus": fx.mk_chorus(
                        time=ctrl("x1"), depth=ctrl("x2"), rate=ctrl("x3")
                    ),
                    "reverb": fx.mk_reverb(time=1.0, dry=0, wet=1),
                },
            )
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
        i = 0
        for splay in pattern:
            i += 1
            params = {
                "start": splay["start"],
                "end": splay["start"] + splay["dur"],
                "pan": rnd.between(-0.1, 0.1),
            }

            for rate in [1, 2, 4]:
                params["rate"] = rate
                sp.play("dynasty-freaking-intro-120", **params)

            sleep_for = splay["dur"] * 16
            sleep(sleep_for)
            slept += sleep_for

        sleep(4 - slept)
