# ABOUTME: Grabs camera frames via RTSP and converts to 1-bit PIMG binary for the Mac client.
# ABOUTME: Uses ffmpeg for RTSP, then resizes, dithers, and packs as big-endian bitmap.

import asyncio
import logging
import struct
from io import BytesIO
from PIL import Image

logger = logging.getLogger(__name__)

SNAPSHOT_WIDTH = 448
SNAPSHOT_HEIGHT = 252


def png_to_pimg(png_data: bytes) -> bytes | None:
    """Convert PNG image bytes to a 1-bit dithered PIMG binary.

    Returns PIMG bytes (6-byte header + bitmap) or None on error.
    """
    try:
        img = Image.open(BytesIO(png_data))
    except Exception:
        logger.warning("Failed to decode snapshot image")
        return None

    img = img.convert("L")
    img = img.resize((SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT), Image.LANCZOS)
    bw = img.convert("1")

    row_bytes = ((SNAPSHOT_WIDTH + 15) // 16) * 2
    pixels = list(bw.get_flattened_data())
    bitmap = bytearray(row_bytes * SNAPSHOT_HEIGHT)

    for row in range(SNAPSHOT_HEIGHT):
        for col in range(SNAPSHOT_WIDTH):
            if pixels[row * SNAPSHOT_WIDTH + col] == 0:
                byte_idx = row * row_bytes + col // 8
                bit_idx = 7 - (col % 8)
                bitmap[byte_idx] |= (1 << bit_idx)

    header = struct.pack(">hhh", SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT, row_bytes)
    return header + bytes(bitmap)


async def grab_rtsp_frame(url: str) -> bytes | None:
    """Grab a single frame from an RTSP stream as PNG bytes via ffmpeg."""
    try:
        proc = await asyncio.create_subprocess_exec(
            "ffmpeg", "-y",
            "-rtsp_transport", "tcp",
            "-i", url,
            "-vframes", "1",
            "-f", "image2pipe", "-vcodec", "png",
            "pipe:1",
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )
        stdout, stderr = await asyncio.wait_for(proc.communicate(), timeout=10)
    except asyncio.TimeoutError:
        logger.warning("RTSP frame grab timed out for %s", url)
        proc.kill()
        return None
    except Exception as e:
        logger.warning("RTSP frame grab failed for %s: %s", url, e)
        return None

    if proc.returncode != 0:
        logger.debug("ffmpeg exited %d for %s: %s", proc.returncode, url,
                      stderr.decode(errors="replace")[:200])
        return None

    if not stdout:
        return None

    return stdout
