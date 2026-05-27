# ABOUTME: PrusaLink local API adapter.
# ABOUTME: Polls /api/v1/status, /api/v1/job, and /api/v1/cameras/snap.

import logging
import httpx
from ..models import PrinterStatus
from ..config import PrinterConfig

logger = logging.getLogger(__name__)

PRUSALINK_STATE_MAP = {
    "IDLE": "idle",
    "PRINTING": "printing",
    "PAUSED": "paused",
    "FINISHED": "finished",
    "STOPPED": "idle",
    "ATTENTION": "attention",
    "BUSY": "printing",
    "ERROR": "error",
    "READY": "idle",
}


async def poll(client: httpx.AsyncClient, cfg: PrinterConfig) -> PrinterStatus:
    auth = httpx.DigestAuth(cfg.username, cfg.password) if cfg.password else None

    status_resp = await client.get(f"{cfg.url}/api/v1/status", auth=auth, timeout=10)
    status_resp.raise_for_status()
    data = status_resp.json()

    printer_data = data.get("printer", {})
    job_data = data.get("job", {})

    raw_state = printer_data.get("state", "IDLE")
    state = PRUSALINK_STATE_MAP.get(raw_state, "offline")

    result = PrinterStatus(
        id=cfg.id,
        name=cfg.name,
        type="prusalink",
        model=cfg.model,
        state=state,
        progress=int(job_data.get("progress", 0)),
        time_remaining=int(job_data.get("time_remaining", 0)),
        nozzle_temp=int(printer_data.get("temp_nozzle", 0)),
        nozzle_target=int(printer_data.get("target_nozzle", 0)),
        bed_temp=int(printer_data.get("temp_bed", 0)),
        bed_target=int(printer_data.get("target_bed", 0)),
    )

    if state in ("printing", "paused") and not job_data.get("progress"):
        pass

    if job_data:
        try:
            job_resp = await client.get(f"{cfg.url}/api/v1/job", auth=auth, timeout=10)
            if job_resp.status_code == 200:
                job_detail = job_resp.json()
                file_info = job_detail.get("file", {})
                result.job = file_info.get("display_name", "") or file_info.get("name", "")
        except (httpx.HTTPError, KeyError):
            pass

    if state == "error":
        result.error = raw_state

    return result


async def poll_snapshot(client: httpx.AsyncClient, cfg: PrinterConfig) -> bytes | None:
    """Fetch a camera snapshot PNG from PrusaLink. Returns PNG bytes or None."""
    auth = httpx.DigestAuth(cfg.username, cfg.password) if cfg.password else None
    try:
        resp = await client.get(f"{cfg.url}/api/v1/cameras/snap", auth=auth, timeout=10)
        if resp.status_code == 200:
            return resp.content
        return None
    except Exception as e:
        logger.debug("Snapshot fetch failed for %s: %s", cfg.id, e)
        return None


def parse_status(data: dict, cfg: PrinterConfig) -> PrinterStatus:
    """Parse a raw /api/v1/status response without making HTTP calls."""
    printer_data = data.get("printer", {})
    job_data = data.get("job", {})

    raw_state = printer_data.get("state", "IDLE")
    state = PRUSALINK_STATE_MAP.get(raw_state, "offline")

    return PrinterStatus(
        id=cfg.id,
        name=cfg.name,
        type="prusalink",
        model=cfg.model,
        state=state,
        progress=int(job_data.get("progress", 0)),
        time_remaining=int(job_data.get("time_remaining", 0)),
        nozzle_temp=int(printer_data.get("temp_nozzle", 0)),
        nozzle_target=int(printer_data.get("target_nozzle", 0)),
        bed_temp=int(printer_data.get("temp_bed", 0)),
        bed_target=int(printer_data.get("target_bed", 0)),
        error=raw_state if state == "error" else "",
    )
