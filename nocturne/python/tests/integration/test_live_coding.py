"""Integration tests for @live decorator functionality."""

from .base import SoirIntegrationTestCase


class TestLiveCoding(SoirIntegrationTestCase):
    """Test @live() decorator and live code reloading."""

    def test_basic_live(self) -> None:
        """Test that @live() decorated functions execute."""
        code = """
@live()
def kick():
    log('hello')
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("hello"))

    def test_basic_live_no_update(self) -> None:
        """Test that pushing same @live() code twice doesn't re-execute."""
        code = """
@live()
def kick():
    log('hello')
"""
        self.engine.push_code(code)
        self.assertTrue(self.engine.wait_for_notification("hello"))

        self.engine.push_code(code)
        self.assertFalse(self.engine.wait_for_notification("hello"))

    def test_basic_live_get_code(self) -> None:
        """Test retrieving code from @live() decorated functions."""
        code = """
@live()
def kick():
    log('hello')
    pass
    log('world')

log(str(_internals.get_live('kick').code))
"""
        self.engine.push_code(code)

        self.assertTrue(
            self.engine.wait_for_notification(
                """\
    log('hello')
    pass
    log('world')
"""
            )
        )

        code2 = """
@live()
def kick():
    log('hello')
    pass
    log('world')
    x = 10
    if x == 42:
        log('42')
    else:
        log('not 42')

log(str(_internals.get_live('kick').code))
"""
        self.engine.push_code(code2)

        expected_code = """\
    log('hello')
    pass
    log('world')
    x = 10
    if x == 42:
        log('42')
    else:
        log('not 42')"""
        self.assertTrue(self.engine.wait_for_notification(expected_code))
