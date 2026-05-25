# PrintWatch — Specification

A Mac System 7 application for monitoring 3D printers on a Mac SE,
backed by a Python proxy service that bridges modern HTTPS printer APIs
to plain HTTP/1.0 that MacTCP can consume.

## Architecture

```
┌──────────────┐  HTTP/1.0   ┌───────────────┐  HTTP/HTTPS  ┌──────────────┐
│   Mac SE     │ ──────────> │  PrintWatch   │ ──────────── │ PrusaLink    │
│  PrintWatch  │  (port      │    Proxy      │              │ (local LAN)  │
│   App        │   8080)     │  (Docker)     │              └──────────────┘
└──────────────┘             │               │  HTTP        ┌──────────────┐
                             │  Python +     │ ──────────── │ Moonraker    │
                             │  Flask/       │              │ (local LAN)  │
                             │  FastAPI      │              └──────────────┘
                             └───────────────┘
                              reads config.yaml
```

The Mac app makes simple `GET` requests to the proxy. The proxy
polls printer APIs, normalizes responses, and serves them as compact
JSON over plain HTTP. The Mac never touches HTTPS or complex auth.

## Component 1: Proxy Service (Python + Docker)

### Configuration (`config.yaml`)

```yaml
proxy:
  host: "0.0.0.0"
  port: 8080
  poll_interval: 10    # seconds between polling each printer

printers:
  - id: "mk4"
    name: "Prusa MK4"
    type: "prusalink"
    url: "http://192.168.1.50"
    username: "maker"
    password: "aBcDeFgH"     # API key from printer LCD

  - id: "voron"
    name: "Voron 2.4"
    type: "moonraker"
    url: "http://192.168.1.60:7125"
    api_key: ""              # empty = trusted client (no auth)

  - id: "ender"
    name: "Ender 3 K1"
    type: "moonraker"
    url: "http://192.168.1.61:7125"
    api_key: "abc123def456"
```

### Proxy REST API

The proxy exposes two endpoints over plain HTTP/1.0-compatible responses.

#### `GET /printers`

Returns all printers with current status. This is the primary polling
endpoint for the Mac app.

```json
{
  "printers": [
    {
      "id": "mk4",
      "name": "Prusa MK4",
      "type": "prusalink",
      "state": "printing",
      "progress": 34,
      "job": "benchy.gcode",
      "time_remaining": 4320,
      "nozzle_temp": 215,
      "nozzle_target": 215,
      "bed_temp": 60,
      "bed_target": 60,
      "error": ""
    },
    {
      "id": "voron",
      "name": "Voron 2.4",
      "type": "moonraker",
      "state": "idle",
      "progress": 0,
      "job": "",
      "time_remaining": 0,
      "nozzle_temp": 22,
      "nozzle_target": 0,
      "bed_temp": 21,
      "bed_target": 0,
      "error": ""
    }
  ]
}
```

Field definitions:

| Field            | Type   | Description                                 |
|------------------|--------|---------------------------------------------|
| `id`             | string | Unique ID from config                       |
| `name`           | string | Human-readable name from config             |
| `type`           | string | `"prusalink"` or `"moonraker"`              |
| `state`          | string | Normalized state (see below)                |
| `progress`       | int    | 0–100 percent complete                      |
| `job`            | string | Current filename, empty if idle             |
| `time_remaining` | int    | Seconds remaining, 0 if unknown/idle        |
| `nozzle_temp`    | int    | Current nozzle temperature (°C)             |
| `nozzle_target`  | int    | Target nozzle temperature (°C)              |
| `bed_temp`       | int    | Current bed temperature (°C)                |
| `bed_target`     | int    | Target bed temperature (°C)                 |
| `error`          | string | Error message if state is `"error"`, else empty |

Normalized states (proxy maps both APIs to these):

| State        | Meaning                               |
|--------------|---------------------------------------|
| `idle`       | Ready, not printing                   |
| `printing`   | Actively printing                     |
| `paused`     | Print paused                          |
| `finished`   | Print complete, not yet cleared       |
| `error`      | Printer error, see `error` field      |
| `attention`  | Needs user action (filament change)   |
| `offline`    | Proxy cannot reach the printer        |

#### `GET /printers/{id}`

Returns a single printer's status (same shape as one element of the
`printers` array above). Returns HTTP 404 if the ID is unknown.

### Proxy Implementation Notes

- Python, using `aiohttp` or `httpx` for async polling
- Lightweight HTTP server (Flask or bare `http.server`) for the Mac-facing API
- Polls each printer at the configured interval in a background task
- Caches the latest response; Mac requests are served from cache (fast)
- Responses use `Connection: close` and `Content-Length` headers for
  HTTP/1.0 compatibility
- Integer temps (no decimals) to keep JSON small and parsing simple
- Progress as integer 0–100 (not float 0.0–1.0)

### PrusaLink API Mapping

The proxy calls these PrusaLink endpoints per printer:

- `GET /api/v1/status` — state, temps, job progress (primary)
- `GET /api/v1/job` — filename and time remaining (when printing)
- `GET /api/v1/info` — printer identity (called once at startup, cached)

Authentication: HTTP Digest (username from config, password = API key).

State mapping from PrusaLink → normalized:

| PrusaLink State | Normalized    |
|-----------------|---------------|
| `IDLE`          | `idle`        |
| `PRINTING`      | `printing`    |
| `PAUSED`        | `paused`      |
| `FINISHED`      | `finished`    |
| `STOPPED`       | `idle`        |
| `ATTENTION`     | `attention`   |
| `BUSY`          | `printing`    |
| `ERROR`         | `error`       |

### Moonraker API Mapping

The proxy calls these Moonraker endpoints per printer:

- `GET /printer/objects/query?print_stats&virtual_sdcard&extruder&heater_bed&display_status`
  — all status in one call
- `GET /printer/info` — hostname, Klipper state (called once at startup)
- `GET /server/files/metadata?filename={name}` — estimated total time
  (called when a new print starts, cached per filename)

Authentication: `X-Api-Key` header if `api_key` is set in config;
otherwise no auth (trusted client).

State mapping from Moonraker → normalized:

| Moonraker `print_stats.state` | Normalized   |
|-------------------------------|--------------|
| `standby`                     | `idle`       |
| `printing`                    | `printing`   |
| `paused`                      | `paused`     |
| `complete`                    | `finished`   |
| `cancelled`                   | `idle`       |
| `error`                       | `error`      |

Time remaining for Moonraker: `metadata.estimated_time - print_stats.print_duration`.
Falls back to progress-based estimate if metadata is unavailable.

## Component 2: Mac Application

### Platform & Toolchain

- **Target:** Mac SE, System 7.0.1, MacTCP 2.0.6
- **CPU:** Motorola 68000 @ 8 MHz
- **Display:** 512×342, 1-bit (black and white)
- **RAM:** 1–4 MB (expect ~600 KB–3 MB available app heap)
- **Toolchain:** Retro68 (GCC cross-compiler for 68K)
- **Language:** C (C89/C99 subset supported by Retro68)
- **Networking:** MacTCP via `antscode/MacTCPHelper` library

### Dependencies

MacTCP headers are not included in Retro68's Multiversal Interfaces.
We use two third-party libraries:

1. **MacTCPHelper** (`github.com/antscode/MacTCPHelper`) — provides
   `MacTCP.h`, DNS resolver, and high-level TCP stream wrappers.
   Must be built and installed into the Retro68 toolchain first.

2. **jsmn** (`github.com/zserge/jsmn`) — single-header JSON tokenizer.
   Zero allocation, parses in-place. Ideal for the constrained 68000
   environment with predictable response schemas.

### Networking Architecture

The app uses synchronous MacTCP calls with a `GiveTime` callback that
calls `WaitNextEvent` to keep the UI responsive during network I/O.

Each poll cycle:
1. `TCPActiveOpen` to the proxy (IP:port configured in app)
2. `TCPSend` the HTTP/1.0 GET request
3. Loop `TCPRcv` until `connectionClosing` or response complete
4. `TCPClose` to send FIN
5. Parse the response body with jsmn
6. Update the UI

The TCP stream is created once at startup and reused across poll cycles.
No DNS resolution needed — proxy address is a configured IP.

HTTP request format:
```
GET /printers HTTP/1.0\r\n
Host: {proxy_ip}\r\n
Connection: close\r\n
\r\n
```

The app uses `HTTP/1.0` exclusively. The proxy guarantees
`Connection: close` and `Content-Length` in responses.

### Configuration

The proxy IP address and port are stored in a Rez `STR#` resource:

- `STR# 128,1` — proxy IP address (e.g., `"192.168.1.100"`)
- `STR# 128,2` — proxy port (e.g., `"8080"`)
- `STR# 128,3` — poll interval in seconds (e.g., `"15"`)

These can be edited with ResEdit on the Mac.

### User Interface

The app has a single window with two views, toggled by the user.

#### List View (default)

```
┌─ PrintWatch ────────────────────────────────┐
│ ┌──────────────────────────────────────────┐ │
│ │►Prusa MK4      Printing  ████████░░  78% │ │
│ │                 benchy.gcode    1:12 rem │ │
│ ├──────────────────────────────────────────┤ │
│ │ Voron 2.4      Idle                      │ │
│ │                                          │ │
│ ├──────────────────────────────────────────┤ │
│ │ Ender 3 K1     Printing  ██░░░░░░░░  18% │ │
│ │                 bracket.gcode   4:30 rem │ │
│ ├──────────────────────────────────────────┤ │
│ │ Prusa XL       ⚠ Attention              │ │
│ │                 Filament change needed   │ │
│ └──────────────────────────────────────────┘ │
│ Last updated: 2:34 PM     [Refresh]         │
└─────────────────────────────────────────────┘
```

Each printer row shows:
- Name (bold/emphasized)
- State
- Progress bar (when printing), drawn with QuickDraw FillRect
- Job filename (when printing)
- Time remaining, formatted as H:MM

Click a printer row or press Enter to switch to Detail View.
The `►` indicator shows the selected row. Arrow keys navigate.

#### Detail View

```
┌─ PrintWatch ─ Prusa MK4 ───────────────────┐
│                                             │
│  State:     Printing                        │
│  Job:       benchy.gcode                    │
│  Progress:  ████████████████░░░░░░░░  78%   │
│  Remaining: 1 hr 12 min                     │
│                                             │
│  ─── Temperatures ──────────────────────    │
│  Nozzle:    215°C / 215°C                   │
│  Bed:        60°C /  60°C                   │
│                                             │
│                                             │
│                                             │
│                                             │
│  Last updated: 2:34 PM                      │
│                          [Back]  [Refresh]  │
└─────────────────────────────────────────────┘
```

Shows all available fields for one printer. Press Escape or click
Back to return to List View.

#### Menus

- **File**: Refresh Now (⌘R), Quit (⌘Q)
- **Edit**: (standard, disabled — needed for DA compatibility)
- **Options**: Set Proxy Address…, Set Poll Interval…

"Set Proxy Address" and "Set Poll Interval" show simple dialogs
that update the STR# 128 resource in the app's resource fork.

#### Visual Design

- 1-bit graphics only (no grays, no dithering)
- Progress bars: solid fill with 1px border
- State indicators: text labels, with "⚠" for attention/error
  (using a custom FOND character or drawn as a small icon)
- Standard System 7 window frame with title bar
- Geneva 9pt for body text, Geneva 12pt bold for printer names
- Use `InvalRect` / `BeginUpdate` / `EndUpdate` for
  flicker-free redraws

### Memory Budget

| Component                | Estimated Size |
|--------------------------|----------------|
| MacTCP driver (system)   | ~35 KB         |
| TCP receive buffer       | 8 KB           |
| HTTP response buffer     | 4 KB           |
| jsmn token array         | 2 KB           |
| Printer data (6 printers)| 1 KB           |
| Application code + UI    | ~30 KB         |
| Stack                    | 32 KB          |
| **Total app footprint**  | **~77 KB**     |

Fits comfortably in a 1 MB Mac SE with System 7.

### Event Loop

The app uses a standard Mac Toolbox event loop:

```
while (!gQuit) {
    WaitNextEvent(everyEvent, &event, pollTicksRemaining, NULL);
    switch (event.what) {
        case mouseDown:   HandleMouseDown(&event); break;
        case keyDown:     HandleKeyDown(&event);   break;
        case updateEvt:   HandleUpdate(&event);    break;
        case activateEvt: HandleActivate(&event);  break;
        case osEvt:       HandleOSEvt(&event);     break;
    }

    if (TickCount() >= nextPollTime) {
        PollPrinters();
        nextPollTime = TickCount() + (pollIntervalSecs * 60);
    }
}
```

`PollPrinters()` does the full HTTP request/parse/redraw cycle.
During the TCP operations, the `GiveTime` callback calls
`WaitNextEvent` to keep the app responsive.

### Error Handling

- **Proxy unreachable:** Show "Cannot reach proxy" in the status bar.
  Continue polling at the normal interval.
- **Malformed response:** Show "Bad response" in status bar, keep
  showing the last valid data.
- **MacTCP not installed:** Show an alert at startup and quit.
  Check with `OpenDriver("\p.IPP", &refNum)`.

## Build System

### Project Structure

```
PrintWatch/
├── SPEC.md
├── CMakeLists.txt
├── src/
│   ├── main.c           # entry point, event loop
│   ├── network.c/.h     # MacTCP HTTP client
│   ├── json.c/.h        # JSON parsing (wraps jsmn)
│   ├── printers.c/.h    # printer data model
│   ├── ui.c/.h          # QuickDraw rendering
│   ├── config.c/.h      # read/write STR# resources
│   └── jsmn.h           # vendored, single header
├── resources/
│   └── PrintWatch.r     # Rez resource definitions
├── proxy/
│   ├── Dockerfile
│   ├── requirements.txt
│   ├── proxy.py          # proxy server
│   └── config.yaml       # example configuration
├── scripts/
│   └── build-and-deploy.sh
└── docs/
    ├── RETRO68_SETUP.md
    ├── EMULATOR_SETUP.md
    └── WORKFLOW.md
```

### CMakeLists.txt (sketch)

```cmake
cmake_minimum_required(VERSION 3.9)
project(PrintWatch C)

add_application(PrintWatch
    TYPE "APPL"
    CREATOR "PrWt"
    resources/PrintWatch.r
    src/main.c
    src/network.c
    src/json.c
    src/printers.c
    src/ui.c
    src/config.c
)

target_link_libraries(PrintWatch MacTCPHelper)
set_target_properties(PrintWatch PROPERTIES
    COMPILE_FLAGS "-Os -ffunction-sections"
    LINK_FLAGS "-Wl,-gc-sections"
)
```

## Testing Strategy

### Proxy

- Unit tests for API normalization (PrusaLink → normalized, Moonraker → normalized)
- Integration tests using recorded API responses (fixture files)
- Manual testing against real printers

### Mac App

- Build and run in Basilisk II (Quadra 900, System 7.5.3) for development
- Run the proxy locally, configure the Mac emulator's network to reach it
- Test with Mini vMac + LaunchAPPL for automated smoke tests
- Final testing on real Mac SE hardware with MacTCP configured

### Network Testing in Emulators

Basilisk II supports SLiRP networking (NAT to host). The emulated Mac
can reach the host machine's Docker proxy. MacTCP must be installed
in the emulated System and configured with a static IP (SLiRP
typically uses 10.0.2.x range).

## Open Questions

1. **MacTCPHelper compatibility:** Need to verify that MacTCPHelper
   builds cleanly against the current Retro68 toolchain. The library
   was last updated for an earlier Retro68 version.

2. **MacHTTP library:** `antscode/MacHTTP` provides a complete HTTP
   client built on MacTCPHelper. If it works, we could use it instead
   of writing our own HTTP layer. Need to evaluate.

3. **Basilisk II networking:** Need to confirm SLiRP works with MacTCP
   in the emulator and that the emulated Mac can reach the host's
   Docker proxy.

4. **PrusaLink vs PrusaConnect:** The spec assumes PrusaLink (local API)
   for Prusa printers since it's well-documented and the proxy runs on
   the same LAN. If cloud access via PrusaConnect is needed, that would
   require reverse-engineering undocumented endpoints.

5. **Multiple extruders:** Some printers (e.g., Prusa XL) have multiple
   nozzles. The current data model tracks one nozzle temp. Extend later
   if needed.
