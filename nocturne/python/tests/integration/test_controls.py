"""Integration tests for controls system (ctrls module)."""

import time

from .base import SoirIntegrationTestCase


class TestControls(SoirIntegrationTestCase):
    """Test ctrls module for creating and managing controls."""

    config_overrides = {"initial_bpm": 600}

    def test_controls_basic(self):
        """Test creating controls in @live() function."""
        self.engine.push_code(
            """
@live()
def controls():
    ctrls.mk_lfo("c1", 0.5)
    ctrls.mk_linear("c2", 0.5, 2.0, 8.0)


log(str(ctrls.layout()))
"""
        )

        time.sleep(1)
        self.assertTrue(self.engine.wait_for_notification("[[c1=0], [c2=0.5]]"))

    def test_controls_global(self):
        """Test creating controls at global scope."""
        self.engine.push_code(
            """
ctrls.mk_lfo("c1", 0.5)
log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

    def test_controls_global_deletion(self):
        """Test that global controls are deleted when not recreated."""
        self.engine.push_code(
            """
ctrls.mk_lfo("c1", 0.5)
log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

        self.engine.push_code("log('empty-eval')")
        self.assertTrue(self.engine.wait_for_notification("empty-eval"))

        time.sleep(1)

        self.engine.push_code(
            """
log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[]"))

    def test_controls_deletion_in_live(self):
        """Test that controls in @live() are deleted when not recreated."""
        self.engine.push_code(
            """
@live()
def setup():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

        self.engine.push_code(
            """
@live()
def setup():
    log('empty-eval')
"""
        )

        self.assertTrue(self.engine.wait_for_notification("empty-eval"))

        time.sleep(1)

        self.engine.push_code(
            """
@live()
def setup():
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[]"))

    def test_controls_live_function_deleted(self):
        """Test that controls are deleted when @live() function is removed."""
        self.engine.push_code(
            """
@live()
def setup():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

        self.engine.push_code(
            """
log('empty-eval')
"""
        )

        self.assertTrue(self.engine.wait_for_notification("empty-eval"))

        time.sleep(1)

        self.engine.push_code(
            """
log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[]"))

    def test_controls_deletion_in_loop(self):
        """Test that controls in @loop() are deleted when not recreated."""
        self.engine.push_code(
            """
@loop(beats=1)
def helloop():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

        self.engine.push_code(
            """
@live()
@loop(beats=1)
def helloop():
    log('empty-eval')
"""
        )

        self.assertTrue(self.engine.wait_for_notification("empty-eval"))

        time.sleep(1)

        self.engine.push_code(
            """
@live()
@loop(beats=1)
def helloop():
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[]"))

    def test_controls_loop_function_deleted(self):
        """Test that controls are deleted when @loop() function is removed."""
        self.engine.push_code(
            """
@loop(beats=1)
def helloop():
    ctrls.mk_lfo("c1", 0.5)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0]]"))

        self.engine.push_code(
            """
log('empty-eval')
"""
        )

        self.assertTrue(self.engine.wait_for_notification("empty-eval"))

        time.sleep(1)

        self.engine.push_code(
            """
log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[]"))

    def test_controls_value(self):
        """Test mk_val() control creation and value updates."""
        self.engine.push_code(
            """
@loop(beats=1)
def helloop():
    ctrls.mk_val("c1", 0.5)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0.5]]"))

        self.engine.push_code(
            """
@live()
def setup():
    ctrls.mk_val("c1", 0.5)

@loop(beats=1)
def helloop():
    log(str(ctrls.layout()))
    sleep(1)
    ctrl("c1").set(0.8)
    log(str(ctrls.layout()))
"""
        )

        self.assertTrue(self.engine.wait_for_notification("[[c1=0.5]]"))
        self.assertTrue(self.engine.wait_for_notification("[[c1=0.8]]"))
