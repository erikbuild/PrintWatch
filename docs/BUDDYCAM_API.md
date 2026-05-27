# BuddyCam / Buddy3D Camera API Reference

The Prusa Core One includes a BuddyCam (Buddy3D Camera) for print monitoring. There are three ways to get images from it.

## 1. PrusaLink API (Recommended)

Snapshot endpoints are exposed on the **printer's** IP, using the same Digest auth as other PrusaLink endpoints (`maker` + API key). No extra dependencies or camera IP needed.

### Endpoints

| Method | Path | Description |
|--------|------|-------------|
| `GET` | `/api/v1/cameras` | List connected cameras (JSON array with IDs) |
| `GET` | `/api/v1/cameras/snap` | Latest cached snapshot from default camera |
| `GET` | `/api/v1/cameras/{id}/snap` | Snapshot from a specific camera |
| `POST` | `/api/v1/cameras/{id}/snap` | Force a fresh capture |

### Response

- **Format**: `image/png` (PNG, not JPEG)
- **200**: Image data returned
- **204**: Camera exists but no image available yet
- **304**: Not modified (use with `If-None-Match`/`ETag` for efficient polling)
- **404**: Camera not found
- **408**: Timeout (POST only — camera didn't respond in time)

### Examples

```bash
# List cameras
curl --digest -u maker:YOUR_API_KEY http://PRINTER_IP/api/v1/cameras

# Get latest snapshot
curl --digest -u maker:YOUR_API_KEY http://PRINTER_IP/api/v1/cameras/snap -o snapshot.png

# Get snapshot from a specific camera
curl --digest -u maker:YOUR_API_KEY http://PRINTER_IP/api/v1/cameras/CAMERA_ID/snap -o snapshot.png

# Force a fresh capture
curl --digest -u maker:YOUR_API_KEY -X POST http://PRINTER_IP/api/v1/cameras/CAMERA_ID/snap -o snapshot.png
```

### Notes

- `GET /snap` returns the most recently cached snapshot. For periodic polling (every 30–60s), this is more reliable than `POST`.
- `POST /snap` forces a new capture but may return 408 if the camera is slow.
- `ETag`/`304` support means the proxy can skip re-processing unchanged images.

## 2. Direct RTSP Stream

The BuddyCam exposes an RTSP stream on its own IP (separate from the printer).

- **URL**: `rtsp://<CAMERA_IP>/live` (port 554)
- **Resolution**: 1920x1080 @ ~25 FPS
- **Auth**: None required
- **Network**: Local only (same subnet)
- **HTTP**: The camera has no HTTP server in stock firmware

### Extracting a still frame

```bash
ffmpeg -y -i rtsp://CAMERA_IP/live -vframes 1 -q:v 1 -f image2 snapshot.jpg
```

Downsides: requires ffmpeg, ~1–2s connection setup latency, and you need the camera's IP in addition to the printer's.

## 3. Prusa Connect Cloud API (Upload Only)

The camera firmware sends periodic snapshots to the cloud. This is **not useful for local retrieval** — included for completeness.

```
PUT https://webcam.connect.prusa3d.com/c/snapshot
Content-Type: image/jpg
fingerprint: <base64 camera identifier>
token: <token from Prusa Connect camera tab>
```

## Sources

- [Buddy3D Camera — Prusa Knowledge Base](https://help.prusa3d.com/article/buddy3d-camera_821264)
- [PrusaLink OpenAPI Spec](https://github.com/prusa3d/Prusa-Link-Web/blob/master/spec/openapi.yaml)
- [Prusa Connect Camera API](https://help.prusa3d.com/article/prusa-connect-camera-api_569012)
