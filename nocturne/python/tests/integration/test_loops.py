"""Integration tests for @loop() decorator functionality."""

from .base import SoirIntegrationTestCase


class TestLoops(SoirIntegrationTestCase):
    """Test @loop() decorator and loop execution."""

    config_overrides = {"initial_bpm": 600}

    def test_basic_loop(self) -> None:
        """Test that @loop() decorated functions execute repeatedly."""
        code = """
i = 0

@loop()
def kick():
    global i
    log('loop ' + str(i))
    i += 1
"""
        self.engine.push_code(code)

        for i in range(10):
            self.assertTrue(self.engine.wait_for_notification(f"loop {i}"))

    def test_basic_loop_failed_update(self) -> None:
        """Test that loop recovers after failed code update."""
        code = """
@loop()
def kick2():
    log('Hello')
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("Hello"))

        bad_code = """
@loop()
def kick2():
    error
"""
        self.engine.push_code(bad_code)

        good_code = """
@loop()
def kick2():
    log('Hello 2')
"""
        self.engine.push_code(good_code)
        self.assertTrue(self.engine.wait_for_notification("Hello 2"))
