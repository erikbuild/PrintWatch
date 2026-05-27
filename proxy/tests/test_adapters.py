# ABOUTME: Tests for PrusaLink and Moonraker API adapters.
# ABOUTME: Uses fixture JSON to verify state mapping, temp extraction, and progress.

import json
import os
from proxy.adapters.prusalink import parse_status, PRUSALINK_STATE_MAP
from proxy.adapters.moonraker import parse_objects_query, MOONRAKER_STATE_MAP
from proxy.config import PrinterConfig

FIXTURES = os.path.join(os.path.dirname(__file__), "fixtures")


def _load_fixture(name: str) -> dict:
    with open(os.path.join(FIXTURES, name)) as f:
        return json.load(f)


def _prusalink_cfg() -> PrinterConfig:
    return PrinterConfig(id="mk4", name="Prusa MK4", type="prusalink",
                         url="http://192.168.1.50", model="MK4S",
                         password="abc")


def _moonraker_cfg() -> PrinterConfig:
    return PrinterConfig(id="voron", name="Voron 2.4", type="moonraker",
                         url="http://192.168.1.60:7125", model="V2.4r2")


class TestPrusaLink:
    def test_model_passed_through(self):
        data = _load_fixture("prusalink_status_printing.json")
        result = parse_status(data, _prusalink_cfg())
        assert result.model == "MK4S"

    def test_printing_state(self):
        data = _load_fixture("prusalink_status_printing.json")
        result = parse_status(data, _prusalink_cfg())
        assert result.state == "printing"
        assert result.progress == 34
        assert result.time_remaining == 4320
        assert result.nozzle_temp == 215
        assert result.nozzle_target == 215
        assert result.bed_temp == 60
        assert result.bed_target == 60

    def test_idle_state(self):
        data = _load_fixture("prusalink_status_idle.json")
        result = parse_status(data, _prusalink_cfg())
        assert result.state == "idle"
        assert result.progress == 0
        assert result.time_remaining == 0
        assert result.nozzle_temp == 22
        assert result.bed_temp == 21

    def test_all_state_mappings(self):
        for raw, expected in PRUSALINK_STATE_MAP.items():
            data = {"printer": {"state": raw}}
            result = parse_status(data, _prusalink_cfg())
            assert result.state == expected, f"{raw} -> {result.state}, expected {expected}"

    def test_error_state_sets_error_field(self):
        data = {"printer": {"state": "ERROR"}}
        result = parse_status(data, _prusalink_cfg())
        assert result.state == "error"
        assert result.error == "ERROR"

    def test_temps_are_integers(self):
        data = _load_fixture("prusalink_status_printing.json")
        result = parse_status(data, _prusalink_cfg())
        assert isinstance(result.nozzle_temp, int)
        assert isinstance(result.bed_temp, int)


class TestMoonraker:
    def test_model_passed_through(self):
        data = _load_fixture("moonraker_objects_printing.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.model == "V2.4r2"

    def test_printing_state(self):
        data = _load_fixture("moonraker_objects_printing.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.state == "printing"
        assert result.progress == 37
        assert result.job == "voron_part.gcode"
        assert result.nozzle_temp == 245
        assert result.nozzle_target == 245
        assert result.bed_temp == 109
        assert result.bed_target == 110

    def test_idle_state(self):
        data = _load_fixture("moonraker_objects_idle.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.state == "idle"
        assert result.progress == 0
        assert result.job == ""
        assert result.nozzle_temp == 22
        assert result.bed_temp == 21

    def test_all_state_mappings(self):
        for raw, expected in MOONRAKER_STATE_MAP.items():
            data = {"result": {"status": {"print_stats": {"state": raw}}}}
            result = parse_objects_query(data, _moonraker_cfg())
            assert result.state == expected, f"{raw} -> {result.state}, expected {expected}"

    def test_error_state_includes_message(self):
        data = {"result": {"status": {
            "print_stats": {"state": "error", "message": "Thermal runaway"},
        }}}
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.state == "error"
        assert result.error == "Thermal runaway"

    def test_progress_from_display_status(self):
        data = _load_fixture("moonraker_objects_printing.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.progress == 37

    def test_time_remaining_estimated_from_progress(self):
        data = _load_fixture("moonraker_objects_printing.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert result.time_remaining > 0

    def test_temps_are_integers(self):
        data = _load_fixture("moonraker_objects_printing.json")
        result = parse_objects_query(data, _moonraker_cfg())
        assert isinstance(result.nozzle_temp, int)
        assert isinstance(result.bed_temp, int)
