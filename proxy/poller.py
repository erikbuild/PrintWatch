# ABOUTME: Background polling engine for printer status.
# ABOUTME: Runs async tasks that periodically poll each printer and update a shared cache.

import asyncio
import logging
import httpx
from .config import ProxyConfig, PrinterConfig
from .models import PrinterStatus
from .adapters import prusalink, moonraker

logger = logging.getLogger(__name__)


class Poller:
    def __init__(self, config: ProxyConfig):
        self.config = config
        self.cache: dict[str, PrinterStatus] = {}
        self._tasks: list[asyncio.Task] = []
        self._metadata_caches: dict[str, dict] = {}

        for p in config.printers:
            self.cache[p.id] = PrinterStatus(
                id=p.id, name=p.name, type=p.type, state="offline"
            )
            if p.type == "moonraker":
                self._metadata_caches[p.id] = {}

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

    async def _poll_loop(self, cfg: PrinterConfig):
        async with httpx.AsyncClient() as client:
            while True:
                try:
                    if cfg.type == "prusalink":
                        status = await prusalink.poll(client, cfg)
                    elif cfg.type == "moonraker":
                        meta_cache = self._metadata_caches.get(cfg.id, {})
                        status = await moonraker.poll(client, cfg, meta_cache)
                    else:
                        status = PrinterStatus(
                            id=cfg.id, name=cfg.name, type=cfg.type, state="offline",
                            error=f"Unknown type: {cfg.type}"
                        )
                    self.cache[cfg.id] = status
                except asyncio.CancelledError:
                    raise
                except Exception as e:
                    logger.warning("Poll failed for %s: %s", cfg.id, e)
                    self.cache[cfg.id] = PrinterStatus(
                        id=cfg.id, name=cfg.name, type=cfg.type,
                        state="offline", error=str(e)
                    )

                await asyncio.sleep(self.config.poll_interval)

    def get_all(self) -> list[dict]:
        return [self.cache[p.id].to_dict() for p in self.config.printers]

    def get_by_id(self, printer_id: str) -> dict | None:
        status = self.cache.get(printer_id)
        return status.to_dict() if status else None
