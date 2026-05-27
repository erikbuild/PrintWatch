# ABOUTME: Tests for YAML config loading and validation.
# ABOUTME: Verifies required fields, defaults, and error handling.

import os
import tempfile
import pytest
from proxy.config import load_config


def _write_config(content: str) -> str:
    fd, path = tempfile.mkstemp(suffix=".yaml")
    with os.fdopen(fd, "w") as f:
        f.write(content)
    return path


def test_load_minimal_config():
    path = _write_config("""
proxy:
  port: 9090
printers:
  - id: mk4
    type: prusalink
    url: http://192.168.1.50
    password: abc123
""")
    config = load_config(path)
    os.unlink(path)
    assert config.port == 9090
    assert config.host == "0.0.0.0"
    assert config.poll_interval == 10
    assert len(config.printers) == 1
    assert config.printers[0].id == "mk4"
    assert config.printers[0].name == "mk4"
    assert config.printers[0].password == "abc123"
    assert config.printers[0].url == "http://192.168.1.50"


def test_load_config_with_name():
    path = _write_config("""
printers:
  - id: voron
    name: "Voron 2.4"
    type: moonraker
    url: http://192.168.1.60:7125
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].name == "Voron 2.4"
    assert config.printers[0].model == ""


def test_load_config_with_model():
    path = _write_config("""
printers:
  - id: mk4
    name: "Prusa MK4"
    type: prusalink
    model: "MK4S"
    url: http://192.168.1.50
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].model == "MK4S"


def test_config_strips_trailing_slash():
    path = _write_config("""
printers:
  - id: mk4
    type: prusalink
    url: http://192.168.1.50/
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].url == "http://192.168.1.50"


def test_config_rejects_missing_id():
    path = _write_config("""
printers:
  - type: prusalink
    url: http://192.168.1.50
""")
    with pytest.raises(ValueError, match="missing required fields"):
        load_config(path)
    os.unlink(path)


def test_config_rejects_unknown_type():
    path = _write_config("""
printers:
  - id: test
    type: octoprint
    url: http://192.168.1.50
""")
    with pytest.raises(ValueError, match="Unknown printer type"):
        load_config(path)
    os.unlink(path)


def test_empty_printers_list():
    path = _write_config("""
proxy:
  port: 8080
printers: []
""")
    config = load_config(path)
    os.unlink(path)
    assert len(config.printers) == 0


def test_camera_defaults_to_false():
    path = _write_config("""
printers:
  - id: mk4
    type: prusalink
    url: http://192.168.1.50
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].camera is False


def test_camera_flag_parsed():
    path = _write_config("""
printers:
  - id: core_one
    type: prusalink
    url: http://192.168.1.50
    camera: true
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].camera is True


def test_camera_url_parsed():
    path = _write_config("""
printers:
  - id: core_one
    type: prusalink
    url: http://192.168.1.50
    camera: true
    camera_url: "rtsp://192.168.1.51/live"
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].camera_url == "rtsp://192.168.1.51/live"


def test_camera_url_defaults_to_empty():
    path = _write_config("""
printers:
  - id: mk4
    type: prusalink
    url: http://192.168.1.50
""")
    config = load_config(path)
    os.unlink(path)
    assert config.printers[0].camera_url == ""
