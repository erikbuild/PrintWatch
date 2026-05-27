# ABOUTME: Moonraker (Klipper) API adapter.
# ABOUTME: Polls /printer/objects/query for combined status, maps to PrinterStatus.

import httpx
from ..models import PrinterStatus
from ..config import PrinterConfig

MOONRAKER_STATE_MAP = {
    "standby": "idle",
    "printing": "printing",
    "paused": "paused",
    "complete": "finished",
    "cancelled": "idle",
    "error": "error",
}

OBJECTS_QUERY = (
    "print_stats"
    "&virtual_sdcard"
    "&extruder"
    "&heater_bed"
    "&display_status"
)


async def poll(client: httpx.AsyncClient, cfg: PrinterConfig,
               metadata_cache: dict = None) -> PrinterStatus:
    headers = {}
    if cfg.api_key:
        headers["X-Api-Key"] = cfg.api_key

    resp = await client.get(
        f"{cfg.url}/printer/objects/query?{OBJECTS_QUERY}",
        headers=headers,
        timeout=10,
    )
    resp.raise_for_status()
    data = resp.json()

    result = parse_objects_query(data, cfg)

    if result.state == "printing" and result.job and metadata_cache is not None:
        if result.job not in metadata_cache:
            try:
                meta_resp = await client.get(
                    f"{cfg.url}/server/files/metadata",
                    params={"filename": result.job},
                    headers=headers,
                    timeout=10,
                )
                if meta_resp.status_code == 200:
                    meta = meta_resp.json().get("result", {})
                    metadata_cache[result.job] = meta.get("estimated_time", 0)
            except httpx.HTTPError:
                metadata_cache[result.job] = 0

        estimated_total = metadata_cache.get(result.job, 0)
        if estimated_total > 0:
            status = data.get("result", {}).get("status", {})
            print_stats = status.get("print_stats", {})
            elapsed = print_stats.get("print_duration", 0)
            result.time_remaining = max(0, int(estimated_total - elapsed))

    return result


def parse_objects_query(data: dict, cfg: PrinterConfig) -> PrinterStatus:
    """Parse a raw /printer/objects/query response without making HTTP calls."""
    status = data.get("result", {}).get("status", {})

    print_stats = status.get("print_stats", {})
    virtual_sdcard = status.get("virtual_sdcard", {})
    extruder = status.get("extruder", {})
    heater_bed = status.get("heater_bed", {})
    display_status = status.get("display_status", {})

    raw_state = print_stats.get("state", "standby")
    state = MOONRAKER_STATE_MAP.get(raw_state, "offline")

    progress_float = display_status.get("progress", virtual_sdcard.get("progress", 0))
    progress = int((progress_float or 0) * 100)

    elapsed = print_stats.get("print_duration", 0)
    time_remaining = 0
    if state == "printing" and progress > 0:
        time_remaining = int(elapsed / (progress / 100.0) * (1 - progress / 100.0))

    return PrinterStatus(
        id=cfg.id,
        name=cfg.name,
        type="moonraker",
        model=cfg.model,
        state=state,
        progress=progress,
        job=print_stats.get("filename", ""),
        time_remaining=time_remaining,
        nozzle_temp=int(extruder.get("temperature", 0)),
        nozzle_target=int(extruder.get("target", 0)),
        bed_temp=int(heater_bed.get("temperature", 0)),
        bed_target=int(heater_bed.get("target", 0)),
        error=print_stats.get("message", "") if state == "error" else "",
    )
