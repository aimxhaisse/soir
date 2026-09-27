"""Unit tests for the _dexed module (Dexed patch DSL)."""

import unittest

from soir.rt import _dexed

KNOWN_VST3_TITLES: set[str] = {
    "Cutoff",
    "Resonance",
    "Output",
    "ALGORITHM",
    "FEEDBACK",
    "OSC KEY SYNC",
    "TRANSPOSE",
    "LFO SPEED",
    "LFO PM DEPTH",
    "LFO AM DEPTH",
    "LFO WAVE",
}
for _op in range(1, 7):
    for _suffix in (
        "F COARSE",
        "F FINE",
        "OUTPUT LEVEL",
        "SWITCH",
        "MODE",
        "OSC DETUNE",
        "RATE SCALING",
        "KEY VELOCITY",
    ):
        KNOWN_VST3_TITLES.add(f"OP{_op} {_suffix}")
    for _stage in range(1, 5):
        KNOWN_VST3_TITLES.add(f"OP{_op} EG RATE {_stage}")
        KNOWN_VST3_TITLES.add(f"OP{_op} EG LEVEL {_stage}")


class TestParams(unittest.TestCase):
    """Test cases for _dexed.params()."""

    def test_all_operators_are_emitted(self) -> None:
        """A patch defines the whole sound: unlisted ops are switched off."""
        p = _dexed.params({"ops": [_dexed.op(440.0)]})
        self.assertEqual(p["OP1 SWITCH"], 1.0)
        for i in range(2, 7):
            self.assertEqual(p[f"OP{i} SWITCH"], 0.0)

    def test_global_params_defaults(self) -> None:
        p = _dexed.params({"ops": [_dexed.op(440.0)]})
        self.assertEqual(p["ALGORITHM"], 0.0)  # algo 1
        self.assertEqual(p["FEEDBACK"], 0.0)
        self.assertEqual(p["OSC KEY SYNC"], 0.0)
        self.assertEqual(p["TRANSPOSE"], 0.5)  # 0 semitones
        self.assertEqual(p["Output"], 0.8)
        self.assertEqual(p["LFO SPEED"], 0.0)
        self.assertEqual(p["LFO AM DEPTH"], 0.0)

    def test_global_params_values(self) -> None:
        p = _dexed.params(
            {
                "algo": 3,
                "feedback": 0.5,
                "cutoff": 12000.0,
                "resonance": 0.25,
                "out": 0.5,
                "transpose": -12,
                "lfo": {"speed": 0.25, "pm": 1.0, "am": 0.0, "wave": 0.5},
                "ops": [_dexed.op(440.0)],
            }
        )
        self.assertEqual(p["ALGORITHM"], 2.0 / 30.0)
        self.assertEqual(p["FEEDBACK"], round(0.5 * 6) / 6.0)
        self.assertEqual(p["Resonance"], 0.25)
        self.assertEqual(p["Output"], 0.5)
        self.assertEqual(p["TRANSPOSE"], 12.0 / 48.0)
        self.assertEqual(p["LFO SPEED"], 0.25)
        self.assertEqual(p["LFO PM DEPTH"], 1.0)
        self.assertEqual(p["LFO WAVE"], 0.5)

    def test_cutoff_scale(self) -> None:
        low = _dexed.params({"cutoff": 20.0, "ops": [_dexed.op(440.0)]})["Cutoff"]
        high = _dexed.params({"cutoff": 20000.0, "ops": [_dexed.op(440.0)]})["Cutoff"]
        mid = _dexed.params({"cutoff": 1000.0, "ops": [_dexed.op(440.0)]})["Cutoff"]
        self.assertEqual(low, 0.0)
        self.assertEqual(high, 1.0)
        self.assertGreater(mid, low)
        self.assertLess(mid, high)

    def test_operator_frequency_mapping(self) -> None:
        # 440 Hz at A4: stage 0 is -12 st, +100 cents -> note itself.
        p = _dexed.params({"ops": [_dexed.op(440.0)]})
        self.assertEqual(p["OP1 F COARSE"], 0.0)
        self.assertEqual(p["OP1 F FINE"], 1.0)

        # 880 Hz: +12 st -> stage 12 is +11 st, +100 cents.
        p = _dexed.params({"ops": [_dexed.op(880.0)]})
        self.assertEqual(p["OP1 F COARSE"], 12.0 / 30.0)
        self.assertEqual(p["OP1 F FINE"], 1.0)

        # 220 Hz: -12 st -> stage 0, +100 cents.
        p = _dexed.params({"ops": [_dexed.op(220.0)]})
        self.assertEqual(p["OP1 F COARSE"], 0.0)
        self.assertEqual(p["OP1 F FINE"], 1.0)

    def test_operator_fields(self) -> None:
        p = _dexed.params(
            {
                "ops": [
                    _dexed.op(
                        440.0,
                        level=0.3,
                        on=False,
                        mode="fm",
                        detune=3,
                        rate_scaling=4,
                        key_velocity=2,
                    )
                ]
            }
        )
        self.assertEqual(p["OP1 OUTPUT LEVEL"], 0.3)
        self.assertEqual(p["OP1 SWITCH"], 0.0)
        self.assertEqual(p["OP1 MODE"], 1.0)
        self.assertEqual(p["OP1 OSC DETUNE"], 3.0 / 13.0)
        self.assertEqual(p["OP1 RATE SCALING"], 4.0 / 7.0)
        self.assertEqual(p["OP1 KEY VELOCITY"], 2.0 / 7.0)

    def test_envelope_stages(self) -> None:
        env = {
            1: (0.5, 1.0),
            2: (0.3, 0.2),
            3: (0.1, 0.2),
            4: (0.2, 0.0),
        }
        p = _dexed.params({"ops": [_dexed.op(440.0, env=env)]})
        self.assertEqual(p["OP1 EG RATE 1"], 0.5)
        self.assertEqual(p["OP1 EG LEVEL 1"], 1.0)
        self.assertEqual(p["OP1 EG RATE 2"], 0.3)
        self.assertEqual(p["OP1 EG LEVEL 3"], 0.2)
        self.assertEqual(p["OP1 EG RATE 4"], 0.2)
        self.assertEqual(p["OP1 EG LEVEL 4"], 0.0)

    def test_missing_envelope_stages_use_defaults(self) -> None:
        p = _dexed.params({"ops": [_dexed.op(440.0, env={1: (0.9, 1.0)})]})
        self.assertEqual(p["OP1 EG RATE 1"], 0.9)
        self.assertEqual(p["OP1 EG RATE 2"], 0.05)
        self.assertEqual(p["OP1 EG LEVEL 2"], 0.0)

    def test_raw_parameter_passthrough(self) -> None:
        p = _dexed.params({"Custom Param": 0.25, "ops": [_dexed.op(440.0)]})
        self.assertEqual(p["Custom Param"], 0.25)

    def test_requires_operators(self) -> None:
        with self.assertRaises(ValueError):
            _dexed.params({})
        with self.assertRaises(ValueError):
            _dexed.params({"ops": []})

    def test_too_many_operators(self) -> None:
        with self.assertRaises(ValueError):
            _dexed.params({"ops": [_dexed.op(440.0)] * 7})

    def test_operator_must_be_a_dict(self) -> None:
        with self.assertRaises(ValueError):
            _dexed.params({"ops": ["nope"]})


class TestOpDefaults(unittest.TestCase):
    """Test cases for _dexed.op()."""

    def test_defaults(self) -> None:
        o = _dexed.op(440.0)
        self.assertEqual(o["freq"], 440.0)
        self.assertEqual(o["level"], 0.8)
        self.assertTrue(o["on"])
        self.assertEqual(o["mode"], "same")
        self.assertEqual(o["detune"], 7)
        self.assertEqual(o["rate_scaling"], 0)
        self.assertEqual(o["key_velocity"], 3)
        self.assertEqual(o["env"], _dexed.swell_env(0.5))


class TestPresets(unittest.TestCase):
    """Test cases for the PRESETS catalog."""

    def test_catalog_has_a_dozen_patches(self) -> None:
        self.assertEqual(len(_dexed.PRESETS), 12)
        self.assertIn(_dexed.DEFAULT_PATCH, _dexed.PRESETS)

    def test_all_presets_convert(self) -> None:
        for name, spec in _dexed.PRESETS.items():
            with self.subTest(preset=name):
                p = _dexed.params(spec)
                unknown = set(p) - KNOWN_VST3_TITLES
                self.assertEqual(
                    unknown, set(), f"{name} uses unknown VST3 titles: {unknown}"
                )
                for key, value in p.items():
                    self.assertTrue(
                        0.0 <= value <= 1.0, f"{name}/{key}={value} out of range"
                    )
                self.assertTrue(
                    any(p[f"OP{i} SWITCH"] == 1.0 for i in range(1, 7)),
                    f"{name} has no operator switched in",
                )


if __name__ == "__main__":
    unittest.main()
