"""Tests for Flask app routes."""

from collections.abc import Generator

import pytest
from flask.testing import FlaskClient

from soir.www.app import create_app


@pytest.fixture
def client() -> Generator[FlaskClient, None, None]:
    """Create test client."""
    app = create_app()
    app.config["TESTING"] = True
    with app.test_client() as client:
        yield client


class TestRoutes:
    """Test all HTTP routes."""

    def test_home_route(self, client: FlaskClient) -> None:
        """Test / route returns 200."""
        response = client.get("/")
        assert response.status_code == 200

    def test_quickstart_route(self, client: FlaskClient) -> None:
        """Test /quickstart route returns 200."""
        response = client.get("/quickstart")
        assert response.status_code == 200

    def test_examples_route(self, client: FlaskClient) -> None:
        """Test /examples route returns 200."""
        response = client.get("/examples")
        assert response.status_code == 200

    def test_reference_route(self, client: FlaskClient) -> None:
        """Test /reference route returns 200."""
        response = client.get("/reference")
        assert response.status_code == 200

    def test_reference_module_route(self, client: FlaskClient) -> None:
        """Test /reference/<module_path> route returns 404."""
        response = client.get("/reference/soir.dsp")
        assert response.status_code == 404

    def test_nonexistent_route(self, client: FlaskClient) -> None:
        """Test nonexistent route returns 404."""
        response = client.get("/nonexistent")
        assert response.status_code == 404
