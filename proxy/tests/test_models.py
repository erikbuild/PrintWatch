# ABOUTME: Tests for the PrinterStatus data model.
# ABOUTME: Verifies serialization, defaults, and field types.

from proxy.models import PrinterStatus


def test_default_state_is_offline():
    p = PrinterStatus()
    assert p.state == "offline"


def test_to_dict_has_all_fields():
    p = PrinterStatus(id="mk4", name="Prusa MK4", type="prusalink")
    d = p.to_dict()
    assert d["id"] == "mk4"
    assert d["name"] == "Prusa MK4"
    assert d["type"] == "prusalink"
    assert d["model"] == ""
    assert d["state"] == "offline"
    assert d["progress"] == 0
    assert d["job"] == ""
    assert d["time_remaining"] == 0
    assert d["nozzle_temp"] == 0
    assert d["nozzle_target"] == 0
    assert d["bed_temp"] == 0
    assert d["bed_target"] == 0
    assert d["error"] == ""


def test_to_dict_values_are_correct_types():
    p = PrinterStatus(
        id="mk4", name="Prusa", type="prusalink",
        state="printing", progress=78, nozzle_temp=215,
    )
    d = p.to_dict()
    assert isinstance(d["progress"], int)
    assert isinstance(d["nozzle_temp"], int)
    assert isinstance(d["time_remaining"], int)


def test_has_snapshot_defaults_to_false():
    p = PrinterStatus()
    assert p.has_snapshot is False


def test_has_snapshot_in_dict():
    p = PrinterStatus(has_snapshot=True)
    d = p.to_dict()
    assert d["has_snapshot"] is True
