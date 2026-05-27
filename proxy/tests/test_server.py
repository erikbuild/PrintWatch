# ABOUTME: Tests for the HTTP/1.0 proxy server.
# ABOUTME: Verifies response format, headers, and routing.

import json
import threading
import urllib.request
from proxy.models import PrinterStatus
from proxy.server import create_server


class FakePoller:
    """Minimal poller substitute for server tests."""
    def __init__(self):
        self.printers = {
            "mk4": PrinterStatus(id="mk4", name="Prusa MK4", type="prusalink",
                                  state="printing", progress=50),
            "voron": PrinterStatus(id="voron", name="Voron 2.4", type="moonraker",
                                    state="idle"),
        }
        self._order = ["mk4", "voron"]
        self.snapshot_data = {}

    def get_all(self):
        return [self.printers[pid].to_dict() for pid in self._order]

    def get_by_id(self, printer_id):
        p = self.printers.get(printer_id)
        return p.to_dict() if p else None

    def get_snapshot(self, printer_id):
        return self.snapshot_data.get(printer_id)


def _start_server():
    poller = FakePoller()
    server = create_server("127.0.0.1", 0, poller)
    port = server.server_address[1]
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    return server, port


def test_get_printers_returns_json():
    server, port = _start_server()
    try:
        resp = urllib.request.urlopen(f"http://127.0.0.1:{port}/printers")
        assert resp.status == 200
        assert "application/json" in resp.headers.get("Content-Type", "")
        data = json.loads(resp.read())
        assert "printers" in data
        assert len(data["printers"]) == 2
        assert data["printers"][0]["id"] == "mk4"
    finally:
        server.shutdown()


def test_get_printers_has_required_headers():
    server, port = _start_server()
    try:
        resp = urllib.request.urlopen(f"http://127.0.0.1:{port}/printers")
        assert resp.headers.get("Content-Length") is not None
        assert resp.headers.get("Connection") == "close"
    finally:
        server.shutdown()


def test_get_single_printer():
    server, port = _start_server()
    try:
        resp = urllib.request.urlopen(f"http://127.0.0.1:{port}/printers/mk4")
        data = json.loads(resp.read())
        assert data["id"] == "mk4"
        assert data["state"] == "printing"
    finally:
        server.shutdown()


def test_get_unknown_printer_returns_404():
    server, port = _start_server()
    try:
        req = urllib.request.Request(f"http://127.0.0.1:{port}/printers/nonexistent")
        try:
            urllib.request.urlopen(req)
            assert False, "Expected 404"
        except urllib.error.HTTPError as e:
            assert e.code == 404
    finally:
        server.shutdown()


def test_unknown_path_returns_404():
    server, port = _start_server()
    try:
        req = urllib.request.Request(f"http://127.0.0.1:{port}/bad")
        try:
            urllib.request.urlopen(req)
            assert False, "Expected 404"
        except urllib.error.HTTPError as e:
            assert e.code == 404
    finally:
        server.shutdown()


def test_get_snapshot_returns_binary():
    server, port = _start_server()
    pimg_data = b"\x01\x40\x00\xB4\x00\x28" + b"\xFF" * 100
    server.RequestHandlerClass.poller.snapshot_data["mk4"] = pimg_data
    try:
        resp = urllib.request.urlopen(f"http://127.0.0.1:{port}/printers/mk4/snapshot")
        assert resp.status == 200
        assert resp.headers.get("Content-Type") == "application/octet-stream"
        assert resp.headers.get("Content-Length") == str(len(pimg_data))
        assert resp.read() == pimg_data
    finally:
        server.shutdown()


def test_get_snapshot_404_when_no_snapshot():
    server, port = _start_server()
    try:
        req = urllib.request.Request(f"http://127.0.0.1:{port}/printers/mk4/snapshot")
        try:
            urllib.request.urlopen(req)
            assert False, "Expected 404"
        except urllib.error.HTTPError as e:
            assert e.code == 404
    finally:
        server.shutdown()


def test_get_snapshot_404_for_unknown_printer():
    server, port = _start_server()
    try:
        req = urllib.request.Request(f"http://127.0.0.1:{port}/printers/fake/snapshot")
        try:
            urllib.request.urlopen(req)
            assert False, "Expected 404"
        except urllib.error.HTTPError as e:
            assert e.code == 404
    finally:
        server.shutdown()
