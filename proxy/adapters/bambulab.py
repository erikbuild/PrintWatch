# ABOUTME: Bambu Lab LAN mode adapter using MQTT over TLS.
# ABOUTME: Connects to the printer's MQTT broker, merges delta updates into cached state.

import json
import logging
import queue
import ssl
import time

import paho.mqtt.client as mqtt

from ..models import PrinterStatus
from ..config import PrinterConfig

logger = logging.getLogger(__name__)

BAMBU_STATE_MAP = {
    "IDLE": "idle",
    "RUNNING": "printing",
    "PAUSE": "paused",
    "FINISH": "finished",
    "FAILED": "error",
}

PUSHALL_INTERVAL = 300


def merge_state(base: dict, delta: dict) -> dict:
    """Merge a delta update into the base state."""
    merged = dict(base)
    merged.update(delta)
    return merged


def parse_report(data: dict, cfg: PrinterConfig) -> PrinterStatus:
    """Parse a Bambu MQTT report payload into a PrinterStatus."""
    p = data.get("print", {})

    raw_state = p.get("gcode_state", "IDLE")
    state = BAMBU_STATE_MAP.get(raw_state, "offline")

    error = ""
    if state == "error":
        error_code = p.get("print_error", 0)
        if error_code:
            error = f"Error code {error_code}"
        else:
            error = "Print failed"

    return PrinterStatus(
        id=cfg.id,
        name=cfg.name,
        type="bambulab",
        model=cfg.model,
        state=state,
        progress=int(p.get("mc_percent", 0)),
        job=p.get("subtask_name", ""),
        time_remaining=int(p.get("mc_remaining_time", 0)) * 60,
        nozzle_temp=int(p.get("nozzle_temper", 0)),
        nozzle_target=int(p.get("nozzle_target_temper", 0)),
        bed_temp=int(p.get("bed_temper", 0)),
        bed_target=int(p.get("bed_target_temper", 0)),
        error=error,
    )


class BambuConnection:
    """Persistent MQTT connection to a Bambu Lab printer."""

    def __init__(self, cfg: PrinterConfig):
        self.cfg = cfg
        self._state: dict = {}
        self._queue: queue.Queue = queue.Queue()
        self._client: mqtt.Client | None = None
        self._connected = False
        self._last_pushall = 0.0

    def connect(self):
        if self._client is not None:
            return

        self._client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self._client.username_pw_set("bblp", self.cfg.password)
        self._client.tls_set(cert_reqs=ssl.CERT_NONE)
        self._client.tls_insecure_set(True)

        self._client.on_connect = self._on_connect
        self._client.on_message = self._on_message
        self._client.on_disconnect = self._on_disconnect

        host = self.cfg.url.replace("http://", "").replace("https://", "").rstrip("/")
        try:
            self._client.connect(host, 8883, keepalive=60)
            self._client.loop_start()
        except Exception as e:
            logger.warning("MQTT connect failed for %s: %s", self.cfg.id, e)
            self._client = None

    def disconnect(self):
        if self._client:
            self._client.loop_stop()
            self._client.disconnect()
            self._client = None
            self._connected = False

    def _on_connect(self, client, userdata, flags, rc, properties=None):
        if rc == 0:
            self._connected = True
            topic = f"device/{self.cfg.serial}/report"
            client.subscribe(topic)
            logger.info("Subscribed to %s", topic)
            self._send_pushall()
        else:
            logger.warning("MQTT connect failed for %s: rc=%d", self.cfg.id, rc)

    def _on_disconnect(self, client, userdata, flags, rc, properties=None):
        self._connected = False
        logger.warning("MQTT disconnected from %s: rc=%d", self.cfg.id, rc)

    def _on_message(self, client, userdata, msg):
        try:
            data = json.loads(msg.payload)
            self._queue.put(data)
        except (json.JSONDecodeError, UnicodeDecodeError) as e:
            logger.warning("Bad MQTT message from %s: %s", self.cfg.id, e)

    def _send_pushall(self):
        if self._client and self._connected:
            topic = f"device/{self.cfg.serial}/request"
            payload = json.dumps({"pushing": {"sequence_id": "0", "command": "pushall"}})
            self._client.publish(topic, payload)
            self._last_pushall = time.monotonic()

    def drain_and_merge(self):
        """Process all queued MQTT messages into the accumulated state."""
        while not self._queue.empty():
            try:
                data = self._queue.get_nowait()
                print_data = data.get("print", {})
                if print_data:
                    self._state = merge_state(self._state, print_data)
            except queue.Empty:
                break

        if time.monotonic() - self._last_pushall > PUSHALL_INTERVAL:
            self._send_pushall()

    def get_status(self, cfg: PrinterConfig) -> PrinterStatus:
        """Return a PrinterStatus from the accumulated state."""
        return parse_report({"print": self._state}, cfg)


async def poll(client, cfg: PrinterConfig,
               connection: BambuConnection) -> PrinterStatus:
    """Poll a Bambu Lab printer by draining queued MQTT messages."""
    connection.connect()
    connection.drain_and_merge()
    return connection.get_status(cfg)
