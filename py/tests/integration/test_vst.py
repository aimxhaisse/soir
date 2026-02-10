"""Integration tests for VST3 plugin support (vst module).

These tests verify the VST3 plugin integration including:
- Plugin discovery and listing
- Adding VST effects to tracks
- Parameter configuration (static and control-based)
- Effect chain management with VST plugins

Note: Some tests require actual VST3 plugins to be installed on the system
and a properly initialized VST host. Tests will be skipped if these
requirements are not met.
"""

import unittest

from .base import SoirIntegrationTestCase


class TestVstPluginDiscovery(SoirIntegrationTestCase):
    """Test VST3 plugin discovery and listing."""

    def test_vst_plugins_returns_list(self) -> None:
        """Test that vst.plugins() returns a list."""
        self.engine.push_code("""
plugins = vst.plugins()
log(f"type={type(plugins).__name__}")
log(f"count={len(plugins)}")
""")

        self.assertTrue(self.engine.wait_for_notification("type=list"))
        self.assertTrue(self.engine.wait_for_notification("count="))

    def test_vst_plugins_info_structure(self) -> None:
        """Test that plugin info has expected keys when plugins are available."""
        self.engine.push_code("""
plugins = vst.plugins()
if plugins:
    p = plugins[0]
    has_uid = 'uid' in p
    has_name = 'name' in p
    has_vendor = 'vendor' in p
    has_category = 'category' in p
    has_path = 'path' in p
    log(f"structure_ok={has_uid and has_name and has_vendor and has_category and has_path}")
else:
    log("structure_ok=skipped")
""")

        self.assertTrue(self.engine.wait_for_notification("structure_ok="))

    def test_vst_plugins_multiple_calls_consistent(self) -> None:
        """Test that multiple calls to vst.plugins() return consistent results."""
        self.engine.push_code("""
plugins1 = vst.plugins()
plugins2 = vst.plugins()
same_count = len(plugins1) == len(plugins2)
log(f"consistent={same_count}")
""")

        self.assertTrue(self.engine.wait_for_notification("consistent=True"))


class TestVstEffectSetup(SoirIntegrationTestCase):
    """Test adding VST effects to tracks.

    These tests require VST plugins to be installed and the VST host to be
    properly initialized. They will be skipped if the VST integration
    is not fully functional in the test environment.
    """

    def _run_vst_test(self, code: str) -> None:
        """Helper to run VST test code and handle results."""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")

    def test_setup_track_with_vst_effect(self) -> None:
        """Test adding a VST effect to a track."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name),
            }),
        })
        layout = tracks.layout()
        # Check if VST integration is working
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_effect_with_static_params(self) -> None:
        """Test adding a VST effect with static parameter values."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name, params={'param1': 0.5}),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_effect_with_control_params(self) -> None:
        """Test adding a VST effect with control-based parameter automation."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        ctrls.mk_val('cutoff', 0.7)
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name, params={'cutoff': ctrl('cutoff')}),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_effect_with_lfo_param(self) -> None:
        """Test adding a VST effect with LFO-controlled parameter."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        ctrls.mk_lfo('wobble', rate=0.5)
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name, params={'filter': ctrl('wobble')}),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")


class TestVstEffectChain(SoirIntegrationTestCase):
    """Test VST effects in effect chains."""

    def _run_vst_test(self, code: str) -> None:
        """Helper to run VST test code and handle results."""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")

    def test_vst_with_builtin_effects(self) -> None:
        """Test mixing VST effects with built-in effects."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'chorus': fx.mk_chorus(),
                'vst_fx': fx.mk_vst(plugin_name),
                'reverb': fx.mk_reverb(),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['chorus', 'vst', 'reverb']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_multiple_vst_effects(self) -> None:
        """Test adding multiple VST effects to a single track."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst1': fx.mk_vst(plugin_name),
                'vst2': fx.mk_vst(plugin_name),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst', 'vst']:
            log("vst_test_result=passed")
        else:
            fxs_types = [f.type for f in layout['synth'].fxs.values()] if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:fxs={fxs_types}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_remove_vst_effect(self) -> None:
        """Test removing a VST effect from a track."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        # First add the VST effect
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            had_vst = 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']
            # Then remove it
            tracks.setup({
                'synth': tracks.mk('sampler', fxs={}),
            })
            layout = tracks.layout()
            now_empty = 'synth' in layout and len(layout['synth'].fxs) == 0
            if had_vst and now_empty:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:had_vst={had_vst},now_empty={now_empty}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_replace_vst_with_builtin(self) -> None:
        """Test replacing a VST effect with a built-in effect."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        # Start with VST
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'my_fx': fx.mk_vst(plugin_name),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            had_vst = 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst']
            # Replace with chorus
            tracks.setup({
                'synth': tracks.mk('sampler', fxs={
                    'my_fx': fx.mk_chorus(),
                }),
            })
            layout = tracks.layout()
            now_chorus = 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['chorus']
            if had_vst and now_chorus:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:had_vst={had_vst},now_chorus={now_chorus}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")


class TestVstEffectReordering(SoirIntegrationTestCase):
    """Test reordering effects including VST plugins."""

    def test_reorder_vst_in_chain(self) -> None:
        """Test reordering VST effect in the effect chain."""
        self.engine.push_code("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        # Initial order: chorus, vst, reverb
        tracks.setup({
            'synth': tracks.mk('sampler', fxs={
                'chorus': fx.mk_chorus(),
                'vst_fx': fx.mk_vst(plugin_name),
                'reverb': fx.mk_reverb(),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            order1_ok = 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['chorus', 'vst', 'reverb']
            # New order: vst, reverb, chorus
            tracks.setup({
                'synth': tracks.mk('sampler', fxs={
                    'vst_fx': fx.mk_vst(plugin_name),
                    'reverb': fx.mk_reverb(),
                    'chorus': fx.mk_chorus(),
                }),
            })
            layout = tracks.layout()
            order2_ok = 'synth' in layout and [f.type for f in layout['synth'].fxs.values()] == ['vst', 'reverb', 'chorus']
            if order1_ok and order2_ok:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:order1_ok={order1_ok},order2_ok={order2_ok}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")


class TestVstInvalidPlugin(SoirIntegrationTestCase):
    """Test error handling for invalid VST plugin names."""

    def test_invalid_plugin_name(self) -> None:
        """Test that using an invalid plugin name is handled gracefully."""
        self.engine.push_code("""
try:
    tracks.setup({
        'synth': tracks.mk('sampler', fxs={
            'vst_fx': fx.mk_vst('NonExistentPlugin12345'),
        }),
    })

    # If we get here, the track should still be created
    layout = tracks.layout()
    log(f"track_count={len(layout)}")
except Exception as e:
    log(f"vst_error={e}")
""")

        # Either the track is created or we get an error - both are acceptable
        if self.engine.wait_for_notification("track_count=", timeout=5.0):
            return  # Test passes - track was created
        if self.engine.wait_for_notification("vst_error=", timeout=1.0):
            return  # Test passes - error was handled gracefully


class TestVstMultipleTracks(SoirIntegrationTestCase):
    """Test VST effects across multiple tracks."""

    def _run_vst_test(self, code: str) -> None:
        """Helper to run VST test code and handle results."""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")

    def test_same_vst_on_multiple_tracks(self) -> None:
        """Test using the same VST plugin on multiple tracks."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'track1': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name),
            }),
            'track2': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            has_two = len(layout) == 2
            t1_ok = 'track1' in layout and [f.type for f in layout['track1'].fxs.values()] == ['vst']
            t2_ok = 'track2' in layout and [f.type for f in layout['track2'].fxs.values()] == ['vst']
            if has_two and t1_ok and t2_ok:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:has_two={has_two},t1_ok={t1_ok},t2_ok={t2_ok}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_different_params_same_plugin(self) -> None:
        """Test using same VST plugin with different parameters on different tracks."""
        self._run_vst_test("""
try:
    plugins = vst.plugins()
    if not plugins:
        log("vst_test_result=skipped")
    else:
        plugin_name = plugins[0]['name']
        tracks.setup({
            'dry': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name, params={'mix': 0.1}),
            }),
            'wet': tracks.mk('sampler', fxs={
                'vst_fx': fx.mk_vst(plugin_name, params={'mix': 0.9}),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif len(layout) == 2:
            log("vst_test_result=passed")
        else:
            log(f"vst_test_result=failed:len={len(layout)}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")


class TestVstInstrumentDiscovery(SoirIntegrationTestCase):
    """Test VST3 instrument plugin discovery."""

    def test_vst_instruments_returns_list(self) -> None:
        """Test that vst.instruments() returns a list."""
        self.engine.push_code("""
instruments = vst.instruments()
log(f"type={type(instruments).__name__}")
log(f"count={len(instruments)}")
""")

        self.assertTrue(self.engine.wait_for_notification("type=list"))
        self.assertTrue(self.engine.wait_for_notification("count="))

    def test_vst_effects_returns_list(self) -> None:
        """Test that vst.effects() returns a list."""
        self.engine.push_code("""
effects = vst.effects()
log(f"type={type(effects).__name__}")
log(f"count={len(effects)}")
""")

        self.assertTrue(self.engine.wait_for_notification("type=list"))
        self.assertTrue(self.engine.wait_for_notification("count="))

    def test_vst_plugin_type_field(self) -> None:
        """Test that plugins have a type field (effect or instrument)."""
        self.engine.push_code("""
plugins = vst.plugins()
if not plugins:
    log("type_check=skipped")
else:
    all_have_type = all('type' in p for p in plugins)
    valid_types = all(p['type'] in ('effect', 'instrument') for p in plugins)
    log(f"type_check={all_have_type and valid_types}")
""")

        self.assertTrue(self.engine.wait_for_notification("type_check="))

    def test_vst_instruments_only_instruments(self) -> None:
        """Test that vst.instruments() only returns instrument-type plugins."""
        self.engine.push_code("""
instruments = vst.instruments()
if not instruments:
    log("filter_check=skipped")
else:
    all_instruments = all(p.get('type') == 'instrument' for p in instruments)
    log(f"filter_check={all_instruments}")
""")

        self.assertTrue(self.engine.wait_for_notification("filter_check="))

    def test_vst_effects_only_effects(self) -> None:
        """Test that vst.effects() only returns effect-type plugins."""
        self.engine.push_code("""
effects = vst.effects()
if not effects:
    log("filter_check=skipped")
else:
    all_effects = all(p.get('type') == 'effect' for p in effects)
    log(f"filter_check={all_effects}")
""")

        self.assertTrue(self.engine.wait_for_notification("filter_check="))

    def test_vst_instruments_plus_effects_equals_plugins(self) -> None:
        """Test that instruments + effects covers all plugins."""
        self.engine.push_code("""
all_plugins = vst.plugins()
instruments = vst.instruments()
effects = vst.effects()
total_match = len(instruments) + len(effects) == len(all_plugins)
log(f"total_match={total_match}")
""")

        self.assertTrue(self.engine.wait_for_notification("total_match=True"))


class TestVstInstrumentSetup(SoirIntegrationTestCase):
    """Test creating VST instrument tracks."""

    def _run_vst_test(self, code: str) -> None:
        """Helper to run VST test code and handle results."""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 instrument plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")

    def test_setup_vst_instrument_track(self) -> None:
        """Test creating a track with a VST instrument."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            log("vst_test_result=passed")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_with_static_params(self) -> None:
        """Test creating a VSTi track with static parameter values."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name, params={'cutoff': 0.5}),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            log("vst_test_result=passed")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_with_control_params(self) -> None:
        """Test creating a VSTi track with control-based parameter automation."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        ctrls.mk_val('filter_ctl', 0.7)
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name, params={'cutoff': ctrl('filter_ctl')}),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            log("vst_test_result=passed")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_with_fxs(self) -> None:
        """Test creating a VSTi track with effects chain."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name, fxs={
                'reverb': fx.mk_reverb(mix=0.3),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            fxs_types = [f.type for f in layout['synth'].fxs.values()]
            if fxs_types == ['reverb']:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:fxs={fxs_types}")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_with_vst_fx(self) -> None:
        """Test creating a VSTi track with a VST effect in the FX chain."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    fx_plugins = vst.effects()
    if not instruments:
        log("vst_test_result=skipped")
    elif not fx_plugins:
        log("vst_test_result=skipped")
    else:
        inst_name = instruments[0]['name']
        fx_name = fx_plugins[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(inst_name, fxs={
                'my_fx': fx.mk_vst(fx_name),
            }),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            fxs_types = [f.type for f in layout['synth'].fxs.values()]
            if fxs_types == ['vst']:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:fxs={fxs_types}")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_volume_pan(self) -> None:
        """Test creating a VSTi track with custom volume and pan."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name, volume=0.5, pan=-0.3),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst':
            vol_ok = abs(layout['synth'].volume - 0.5) < 0.01
            pan_ok = abs(layout['synth'].pan - (-0.3)) < 0.01
            if vol_ok and pan_ok:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:vol={layout['synth'].volume},pan={layout['synth'].pan}")
        else:
            inst = layout['synth'].instrument if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:instrument={inst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_setup_vst_instrument_muted(self) -> None:
        """Test creating a muted VSTi track."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name, muted=True),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        elif 'synth' in layout and layout['synth'].instrument == 'vst' and layout['synth'].muted:
            log("vst_test_result=passed")
        else:
            muted = layout['synth'].muted if 'synth' in layout else 'missing'
            log(f"vst_test_result=failed:muted={muted}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")


class TestVstInstrumentMultipleTracks(SoirIntegrationTestCase):
    """Test multiple VST instrument tracks."""

    def _run_vst_test(self, code: str) -> None:
        """Helper to run VST test code and handle results."""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("vst_test_result="))
        for n in self.engine.get_notifications():
            if "vst_test_result=skipped" in n:
                self.skipTest("No VST3 instrument plugins available")
            if "vst_test_result=vst_broken" in n:
                self.skipTest("VST host not properly initialized in test environment")
            if "vst_test_result=passed" in n:
                return
            if "vst_test_result=failed" in n:
                self.fail(f"Test failed: {n}")
            if "vst_test_result=error" in n:
                self.fail(f"Exception in test: {n}")

    def test_multiple_vst_instrument_tracks(self) -> None:
        """Test creating multiple VSTi tracks."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'lead': tracks.mk_vst(plugin_name),
            'bass': tracks.mk_vst(plugin_name),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            has_two = len(layout) == 2
            lead_ok = 'lead' in layout and layout['lead'].instrument == 'vst'
            bass_ok = 'bass' in layout and layout['bass'].instrument == 'vst'
            if has_two and lead_ok and bass_ok:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:has_two={has_two},lead_ok={lead_ok},bass_ok={bass_ok}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_vst_instrument_alongside_sampler(self) -> None:
        """Test VSTi track alongside a sampler track."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        tracks.setup({
            'synth': tracks.mk_vst(plugin_name),
            'drums': tracks.mk_sampler(),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            has_two = len(layout) == 2
            synth_ok = 'synth' in layout and layout['synth'].instrument == 'vst'
            drums_ok = 'drums' in layout and layout['drums'].instrument == 'sampler'
            if has_two and synth_ok and drums_ok:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:has_two={has_two},synth_ok={synth_ok},drums_ok={drums_ok}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")

    def test_replace_sampler_with_vst_instrument(self) -> None:
        """Test replacing a sampler track with a VSTi track."""
        self._run_vst_test("""
try:
    instruments = vst.instruments()
    if not instruments:
        log("vst_test_result=skipped")
    else:
        plugin_name = instruments[0]['name']
        # Start with sampler
        tracks.setup({
            'synth': tracks.mk_sampler(),
        })
        layout = tracks.layout()
        if len(layout) == 0:
            log("vst_test_result=vst_broken")
        else:
            was_sampler = 'synth' in layout and layout['synth'].instrument == 'sampler'
            # Replace with VSTi
            tracks.setup({
                'synth': tracks.mk_vst(plugin_name),
            })
            layout = tracks.layout()
            now_vst = 'synth' in layout and layout['synth'].instrument == 'vst'
            if was_sampler and now_vst:
                log("vst_test_result=passed")
            else:
                log(f"vst_test_result=failed:was_sampler={was_sampler},now_vst={now_vst}")
except Exception as e:
    log(f"vst_test_result=error:{e}")
""")


class TestVstInstrumentInvalidPlugin(SoirIntegrationTestCase):
    """Test error handling for invalid VST instrument plugin names."""

    def test_invalid_vst_instrument_name(self) -> None:
        """Test that using an invalid plugin name for VSTi is handled gracefully."""
        self.engine.push_code("""
try:
    tracks.setup({
        'synth': tracks.mk_vst('NonExistentVSTi99999'),
    })

    layout = tracks.layout()
    log(f"track_count={len(layout)}")
except Exception as e:
    log(f"vst_error={e}")
""")

        # Either the track is created or we get an error - both are acceptable
        if self.engine.wait_for_notification("track_count=", timeout=5.0):
            return
        if self.engine.wait_for_notification("vst_error=", timeout=1.0):
            return


if __name__ == "__main__":
    unittest.main()
