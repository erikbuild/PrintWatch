# ABOUTME: Tests for PNG-to-PIMG snapshot conversion and RTSP frame grabbing.
# ABOUTME: Verifies dithering, resizing, header format, bit packing, and ffmpeg invocation.

import asyncio
import struct
from io import BytesIO
from unittest.mock import AsyncMock, patch, MagicMock
import pytest
from PIL import Image
from proxy.snapshot import png_to_pimg, grab_rtsp_frame, SNAPSHOT_WIDTH, SNAPSHOT_HEIGHT


def _make_png(width: int, height: int, color: int = 128) -> bytes:
    """Create a solid-color PNG in memory."""
    img = Image.new("RGB", (width, height), (color, color, color))
    buf = BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


def _make_gradient_png(width: int, height: int) -> bytes:
    """Create a horizontal gradient PNG (black to white)."""
    img = Image.new("L", (width, height))
    for x in range(width):
        val = int(255 * x / (width - 1))
        for y in range(height):
            img.putpixel((x, y), val)
    buf = BytesIO()
    img.save(buf, format="PNG")
    return buf.getvalue()


class TestPngToPimg:
    def test_output_has_valid_header(self):
        png_data = _make_png(1920, 1080)
        pimg = png_to_pimg(png_data)

        width, height, row_bytes = struct.unpack(">hhh", pimg[:6])
        assert width == SNAPSHOT_WIDTH
        assert height == SNAPSHOT_HEIGHT
        assert row_bytes == ((SNAPSHOT_WIDTH + 15) // 16) * 2

    def test_output_size_matches_header(self):
        png_data = _make_png(1920, 1080)
        pimg = png_to_pimg(png_data)

        width, height, row_bytes = struct.unpack(">hhh", pimg[:6])
        expected_size = 6 + row_bytes * height
        assert len(pimg) == expected_size

    def test_white_image_produces_zero_bitmap(self):
        png_data = _make_png(320, 180, color=255)
        pimg = png_to_pimg(png_data)

        bitmap = pimg[6:]
        assert all(b == 0 for b in bitmap), "White image should have no set bits"

    def test_black_image_produces_all_set_bitmap(self):
        png_data = _make_png(320, 180, color=0)
        pimg = png_to_pimg(png_data)

        width, height, row_bytes = struct.unpack(">hhh", pimg[:6])
        bitmap = pimg[6:]
        for row in range(height):
            for col in range(width):
                byte_idx = row * row_bytes + col // 8
                bit_idx = 7 - (col % 8)
                assert (bitmap[byte_idx] >> bit_idx) & 1 == 1, \
                    f"Black pixel expected at ({col}, {row})"

    def test_resizes_large_input(self):
        png_data = _make_png(3840, 2160)
        pimg = png_to_pimg(png_data)

        width, height, _ = struct.unpack(">hhh", pimg[:6])
        assert width == SNAPSHOT_WIDTH
        assert height == SNAPSHOT_HEIGHT

    def test_resizes_small_input(self):
        png_data = _make_png(160, 90)
        pimg = png_to_pimg(png_data)

        width, height, _ = struct.unpack(">hhh", pimg[:6])
        assert width == SNAPSHOT_WIDTH
        assert height == SNAPSHOT_HEIGHT

    def test_gradient_has_mixed_bits(self):
        """A gradient should produce both black and white pixels after dithering."""
        png_data = _make_gradient_png(320, 180)
        pimg = png_to_pimg(png_data)

        bitmap = pimg[6:]
        set_bits = sum(bin(b).count("1") for b in bitmap)
        total_bits = SNAPSHOT_WIDTH * SNAPSHOT_HEIGHT
        assert 0 < set_bits < total_bits, \
            "Gradient should have a mix of black and white pixels"

    def test_returns_none_for_invalid_input(self):
        result = png_to_pimg(b"not a png")
        assert result is None


class TestGrabRtspFrame:
    @pytest.mark.asyncio
    async def test_returns_png_bytes_on_success(self):
        png_data = _make_png(320, 180)
        mock_proc = AsyncMock()
        mock_proc.communicate.return_value = (png_data, b"")
        mock_proc.returncode = 0

        with patch("proxy.snapshot.asyncio.create_subprocess_exec",
                    return_value=mock_proc) as mock_exec:
            result = await grab_rtsp_frame("rtsp://10.0.1.172/live")

        assert result == png_data
        mock_exec.assert_called_once()
        args = mock_exec.call_args[0]
        assert args[0] == "ffmpeg"
        assert "rtsp://10.0.1.172/live" in args

    @pytest.mark.asyncio
    async def test_returns_none_on_ffmpeg_failure(self):
        mock_proc = AsyncMock()
        mock_proc.communicate.return_value = (b"", b"Connection refused")
        mock_proc.returncode = 1

        with patch("proxy.snapshot.asyncio.create_subprocess_exec",
                    return_value=mock_proc):
            result = await grab_rtsp_frame("rtsp://bad-host/live")

        assert result is None

    @pytest.mark.asyncio
    async def test_returns_none_on_timeout(self):
        mock_proc = AsyncMock()
        mock_proc.communicate.side_effect = asyncio.TimeoutError()
        mock_proc.kill = MagicMock()

        with patch("proxy.snapshot.asyncio.create_subprocess_exec",
                    return_value=mock_proc):
            result = await grab_rtsp_frame("rtsp://slow-host/live")

        assert result is None
        mock_proc.kill.assert_called_once()

    @pytest.mark.asyncio
    async def test_returns_none_on_exec_error(self):
        with patch("proxy.snapshot.asyncio.create_subprocess_exec",
                    side_effect=FileNotFoundError("ffmpeg not found")):
            result = await grab_rtsp_frame("rtsp://10.0.1.172/live")

        assert result is None
