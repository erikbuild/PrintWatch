# ABOUTME: HTTP/1.0 server for the PrintWatch proxy.
# ABOUTME: Serves normalized printer status JSON to classic Mac clients.

import json
import logging
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from .poller import Poller

logger = logging.getLogger(__name__)


class ProxyHandler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.0"
    server_version = "PrintWatchProxy/1.0"
    poller: "Poller" = None

    def do_GET(self):
        if self.path == "/printers":
            self._serve_all_printers()
        elif self.path.startswith("/printers/"):
            printer_id = self.path[len("/printers/"):]
            self._serve_single_printer(printer_id)
        else:
            self.send_error(404, "Not Found")

    def _serve_all_printers(self):
        data = {"printers": self.poller.get_all()}
        self._send_json(data)

    def _serve_single_printer(self, printer_id: str):
        data = self.poller.get_by_id(printer_id)
        if data is None:
            self.send_error(404, "Printer not found")
            return
        self._send_json(data)

    def _send_json(self, data):
        body = json.dumps(data, separators=(",", ":")).encode("ascii")
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, format, *args):
        logger.debug(format, *args)


def create_server(host: str, port: int, poller: "Poller") -> HTTPServer:
    ProxyHandler.poller = poller
    server = HTTPServer((host, port), ProxyHandler)
    return server
