# ABOUTME: Background polling engine for printer status.
# ABOUTME: Runs async tasks that periodically poll each printer and update a shared cache.

import asyncio
import logging
import httpx
from .config import ProxyConfig, PrinterConfig
from .models import PrinterStatus
from .model_names import MODEL_NAMES
from .adapters import prusalink, moonraker, bambulab

logger = logging.getLogger(__name__)


def _resolve_model_name(model: str) -> str:
    return MODEL_NAMES.get(model, model)


class Poller:
    def __init__(self, config: ProxyConfig):
        self.config = config
        self.cache: dict[str, PrinterStatus] = {}
        self._tasks: list[asyncio.Task] = []
        self._metadata_caches: dict[str, dict] = {}
        self._bambu_connections: dict[str, bambulab.BambuConnection] = {}

        for p in config.printers:
            self.cache[p.id] = PrinterStatus(
                id=p.id, name=p.name, type=p.type, model=p.model,
                model_name=_resolve_model_name(p.model), state="offline"
            )
            if p.type == "moonraker":
                self._metadata_caches[p.id] = {}
            elif p.type == "bambulab":
                self._bambu_connections[p.id] = bambulab.BambuConnection(p)

    async def start(self):
        for p in self.config.printers:
            task = asyncio.create_task(self._poll_loop(p))
            self._tasks.append(task)
        logger.info("Poller started for %d printers", len(self.config.printers))

    async def stop(self):
        for task in self._tasks:
            task.cancel()
        await asyncio.gather(*self._tasks, return_exceptions=True)
        self._tasks.clear()
        for conn in self._bambu_connections.values():
            conn.disconnect()

    async def _poll_loop(self, cfg: PrinterConfig):
        async with httpx.AsyncClient() as client:
            while True:
                try:
                    if cfg.type == "prusalink":
                        status = await prusalink.poll(client, cfg)
                    elif cfg.type == "moonraker":
                        meta_cache = self._metadata_caches.get(cfg.id, {})
                        status = await moonraker.poll(client, cfg, meta_cache)
                    elif cfg.type == "bambulab":
                        conn = self._bambu_connections[cfg.id]
                        status = await bambulab.poll(client, cfg, conn)
                    else:
                        status = PrinterStatus(
                            id=cfg.id, name=cfg.name, type=cfg.type, state="offline",
                            error=f"Unknown type: {cfg.type}"
                        )
                    status.model_name = _resolve_model_name(cfg.model)
                    self.cache[cfg.id] = status
                except asyncio.CancelledError:
                    raise
                except Exception as e:
                    logger.warning("Poll failed for %s: %s", cfg.id, e)
                    self.cache[cfg.id] = PrinterStatus(
                        id=cfg.id, name=cfg.name, type=cfg.type,
                        model=cfg.model, model_name=_resolve_model_name(cfg.model),
                        state="offline", error=str(e)
                    )

                await asyncio.sleep(self.config.poll_interval)

    def get_all(self) -> list[dict]:
        return [self.cache[p.id].to_dict() for p in self.config.printers]

    def get_by_id(self, printer_id: str) -> dict | None:
        status = self.cache.get(printer_id)
        return status.to_dict() if status else None
