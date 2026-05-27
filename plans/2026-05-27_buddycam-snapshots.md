# BuddyCam Snapshot Integration

## Context

PrintWatch monitors 3D printers from a Mac SE. Prusa Core One printers have a BuddyCam that exposes snapshots via PrusaLink's local API (`GET /api/v1/cameras/snap`). We want to display these camera snapshots on the 1-bit Mac SE display as a dedicated camera view.

The proxy already polls PrusaLink for status using Digest auth over httpx. The Mac client already renders 1-bit PIMG bitmaps via CopyBits (for printer model icons). This feature extends both sides.

## Design Decisions

- **Camera view**: Third view mode (List → Detail → Camera via `C` key). Full content area for the snapshot. Esc goes back to detail.
- **Auto-refresh**: Proxy polls camera on its own interval, caches latest snapshot. Mac client fetches cached snapshot each status poll cycle.
- **Image pipeline**: Proxy fetches PNG from PrusaLink, converts to 1-bit dithered PIMG binary, serves via new endpoint. Mac draws it directly with CopyBits.
- **Image size**: 320×180 (16:9 matching BuddyCam's 1080p aspect ratio). 1-bit packed = 40 bytes/row × 180 = 7,206 bytes. Fits well in the ~486×280 content area.
- **Scope**: PrusaLink printers only (Core One). Other adapters can add camera support later.

## Changes

### Phase 1: Proxy — Snapshot Polling & Serving

#### 1a. Config: add `camera` flag

**`proxy/config.py`** — Add `camera: bool = False` to `PrinterConfig`. Parse from YAML.

**`proxy/config.example.yaml`** — Add `camera: true` to the PrusaLink example entry.

#### 1b. Model: add `has_snapshot` field

**`proxy/models.py`** — Add `has_snapshot: bool = False` to `PrinterStatus`. This tells the Mac client a snapshot is available.

#### 1c. PrusaLink adapter: fetch snapshot

**`proxy/adapters/prusalink.py`** — Add `async def poll_snapshot(client, cfg) -> bytes | None`:
- `GET {cfg.url}/api/v1/cameras/snap` with Digest auth
- Return PNG bytes on 200, `None` on 204/304/error
- Use ETag caching to skip unchanged images

#### 1d. Image conversion module

**`proxy/snapshot.py`** (new) — Convert PNG to 1-bit PIMG binary:
- Use Pillow (already a dependency) to open PNG
- Resize to 320×180 (LANCZOS)
- Convert to grayscale, apply Floyd-Steinberg dithering (`convert("1")` with dithering)
- Pack into PIMG format: 6-byte header `[width:2B][height:2B][rowBytes:2B]` + bitmap data
- Reuse the bit-packing logic from `scripts/generate_icons.py` (`png_to_bitmap`)
- Pack header as big-endian shorts (Mac 68K byte order)

#### 1e. Poller: snapshot poll loop

**`proxy/poller.py`** — Add snapshot caching:
- New dict: `self.snapshot_cache: dict[str, bytes] = {}` (printer_id → PIMG binary)
- For printers with `cfg.camera == True` and `cfg.type == "prusalink"`:
  - Poll snapshot every `poll_interval` alongside status (or use a separate longer interval, configurable later)
  - On new PNG: convert via `snapshot.py`, store PIMG bytes in `snapshot_cache`
  - Set `status.has_snapshot = True` when a snapshot is cached
- New method: `get_snapshot(printer_id) -> bytes | None`

#### 1f. Server: snapshot endpoint

**`proxy/server.py`** — Add `GET /printers/{id}/snapshot`:
- Parse path: if it matches `/printers/{id}/snapshot`, serve binary PIMG
- Call `poller.get_snapshot(id)`
- Return with `Content-Type: application/octet-stream`, `Content-Length`, `Connection: close`
- 404 if no snapshot cached

### Phase 2: Mac Client — Camera View

#### 2a. Network: binary GET with larger buffer

**`src/network.h`** — Add `Net_HttpGetBuf`:
```c
#define NET_SNAPSHOT_BUF_SIZE 10240

int Net_HttpGetBuf(NetState *net, unsigned long ip, short port,
                   const char *host, const char *path,
                   char *buf, int bufSize, int *outLen);
```

**`src/network.c`** — Implement `Net_HttpGetBuf`:
- Same as `Net_HttpGet` but writes into caller-provided buffer instead of `net->httpBuf`
- Strips HTTP headers, writes only body to `buf`, sets `outLen` to body length
- Needed because `NET_HTTP_BUF_SIZE` (4KB) is too small for a 320×180 PIMG (~7.2KB)

#### 2b. Data model: add `has_snapshot` field

**`src/printers.h`** — Add `int has_snapshot;` to `PrinterStatus`.

**`src/json.c`** — Parse `"has_snapshot"` field (boolean → int 0/1).

#### 2c. Snapshot cache

**`src/snapshot.h`** (new) — Snapshot fetch and cache:
```c
#define SNAPSHOT_MAX_SIZE 10240

/* Fetch snapshot for the given printer from the proxy. */
int Snapshot_Fetch(NetState *net, unsigned long ip, short port,
                   const char *host, const char *printerId);

/* Draw the cached snapshot at position (x, y). Returns 0 if no snapshot. */
int Snapshot_Draw(int x, int y);

/* Returns 1 if a snapshot is cached. */
int Snapshot_HasData(void);
```

**`src/snapshot.c`** (new):
- Static buffer: `static char gSnapshotBuf[SNAPSHOT_MAX_SIZE]`
- Static length: `static int gSnapshotLen = 0`
- `Snapshot_Fetch`: calls `Net_HttpGetBuf` for `/printers/{id}/snapshot`, stores in buffer
- `Snapshot_Draw`: parses PIMG header from buffer, renders via `CopyBits` (same pattern as `Icons_Draw`)
- `Snapshot_HasData`: returns `gSnapshotLen > 0`

#### 2d. UI: camera view

**`src/ui.h`** — Add `kViewCamera = 2`.

**`src/ui.c`** — Add `UI_DrawCameraView(WindowPtr window, PrinterStatus *printer)`:
- Header: printer name + "Camera" label
- Draw snapshot centered in content area via `Snapshot_Draw`
- Footer: "Press Esc to go back"

#### 2e. Main: camera view navigation & snapshot polling

**`src/main.c`**:
- Add `kViewCamera` handling to `DrawWindow`
- Key handling: `C` key in detail view → enter camera view (only if `has_snapshot`)
- Esc in camera view → back to detail view
- On each successful status poll: if current printer `has_snapshot`, fetch snapshot via `Snapshot_Fetch`
- Add `snapshot.c` to CMakeLists.txt

### Phase 3: Config & Build

**`CMakeLists.txt`** — Add `src/snapshot.c` to `add_application`.

**`proxy/config.example.yaml`** — Update example:
```yaml
printers:
  - id: "core_one"
    name: "Core One"
    type: "prusalink"
    model: "core_one"
    url: "http://192.168.1.50"
    username: "maker"
    password: "your-api-key"
    camera: true    # Enable BuddyCam snapshots
```

## Size Budget

| Item | Bytes |
|------|-------|
| 320×180 PIMG (1-bit) | ~7,206 |
| HTTP headers overhead | ~200 |
| Snapshot buffer (Mac) | 10,240 |
| Current app memory budget | 204,800 |

The snapshot buffer adds ~10KB to the app's memory footprint. Well within the 200KB SIZE resource.

## Verification

1. **Proxy standalone**: Add `camera: true` to a Core One config entry. Run proxy, curl `GET /printers` and verify `has_snapshot: true`. Curl `GET /printers/{id}/snapshot` and verify PIMG binary (check first 6 bytes = valid width/height/rowBytes).
2. **Image quality**: Save the PIMG output, convert back to PNG with a test script, verify the dithering looks reasonable.
3. **Mac client**: Build, run in BasiliskII. Navigate to detail view for a camera-enabled printer. Press `C` to enter camera view. Verify snapshot renders. Press Esc to return.
4. **Proxy tests**: Mock PrusaLink `/api/v1/cameras/snap` responses, test the PNG→PIMG conversion pipeline, test 204/304/error handling.
5. **No-camera printers**: Verify printers without `camera: true` don't show `has_snapshot` and `C` key does nothing in their detail view.

## Implementation Order

1. `proxy/snapshot.py` — Image conversion (can test independently)
2. `proxy/config.py` + `proxy/models.py` — Config and model changes
3. `proxy/adapters/prusalink.py` — Snapshot polling
4. `proxy/poller.py` — Snapshot cache integration
5. `proxy/server.py` — Snapshot endpoint
6. Proxy tests
7. `src/network.c/h` — Binary GET
8. `src/snapshot.c/h` — Mac snapshot cache + draw
9. `src/printers.h` + `src/json.c` — has_snapshot field
10. `src/ui.c/h` — Camera view rendering
11. `src/main.c` — Navigation + polling integration
12. `CMakeLists.txt` — Add snapshot.c
