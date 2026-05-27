# ABOUTME: Converts camera PNG snapshots to 1-bit PIMG binary for the Mac client.
# ABOUTME: Resizes to fixed dimensions, applies Floyd-Steinberg dithering, packs as big-endian bitmap.

import logging
import struct
from io import BytesIO
from PIL import Image

logger = logging.getLogger(__name__)

SNAPSHOT_WIDTH = 320
SNAPSHOT_HEIGHT = 180


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
