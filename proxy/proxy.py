# ABOUTME: Main entry point for the PrintWatch proxy service.
# ABOUTME: Starts the background poller and HTTP server.

import asyncio
import logging
import sys
import threading
from .config import load_config
from .poller import Poller
from .server import create_server

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)


def run_http_server(server):
    logger.info("HTTP server listening on %s:%d", *server.server_address)
    server.serve_forever()


async def main(config_path: str):
    config = load_config(config_path)
    logger.info("Loaded config: %d printers, poll every %ds",
                len(config.printers), config.poll_interval)

    poller = Poller(config)
    await poller.start()

    server = create_server(config.host, config.port, poller)
    server_thread = threading.Thread(target=run_http_server, args=(server,), daemon=True)
    server_thread.start()

    try:
        while True:
            await asyncio.sleep(1)
    except (KeyboardInterrupt, asyncio.CancelledError):
        logger.info("Shutting down...")
        server.shutdown()
        await poller.stop()


def cli():
    config_path = sys.argv[1] if len(sys.argv) > 1 else "config.yaml"
    asyncio.run(main(config_path))


if __name__ == "__main__":
    cli()
