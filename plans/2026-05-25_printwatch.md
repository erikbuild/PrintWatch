# PrintWatch Implementation Plan
**Date:** 2026-05-25

## Context

PrintWatch is a two-component system: a Python proxy (Docker) that polls PrusaLink and Moonraker APIs for 3D printer status, and a Mac System 7 app (C, Retro68) that displays that status on a Mac SE's 512×342 1-bit display. The proxy bridges HTTPS/auth to plain HTTP/1.0 that MacTCP can consume. See `SPEC.md` for full architecture and API details.

The repo has documentation, emulator setup, and build scripts but no source code yet.

## Critical Risks (address early)

1. **MacTCPHelper compatibility** — must build against current Retro68 GCC 12.2.0. Fallback: write minimal MacTCP wrapper using Retro68's own `LaunchAPPL/Server/MacTCPStream.cc` as reference.
2. **SLiRP networking in Basilisk II** — MacTCP must connect through SLiRP NAT to reach the host's proxy. Poorly documented. Mac sees host at 10.0.2.2.
3. **MacTCP availability** — System 7.5.3 in Basilisk II may ship with Open Transport instead. May need to install MacTCP separately.

---

## Phase 0: Project Scaffolding

### 0.1 Directory structure
Create `src/`, `resources/`, `proxy/`, `tests/`, `plans/`. Update `.gitignore` for Python (`__pycache__/`, `.venv/`) and build artifacts.

### 0.2 Verify Retro68 builds a minimal app
Create `CMakeLists.txt` and `src/main.c` — bare Toolbox init, open a window, draw "PrintWatch", wait for click, quit. Create minimal `resources/PrintWatch.r` with SIZE resource. Build with:
```
cmake_minimum_required(VERSION 3.9)
project(PrintWatch C)
add_application(PrintWatch resources/PrintWatch.r src/main.c)
```
Note: `add_application()` takes a flat file list, NOT keyword args like SOURCES. It separates `.r` files automatically. Deploy `.bin` to `~/Code/Basilisk II/shared/` and verify it runs.

### 0.3 Host-side C test harness
Create `tests/Makefile` that compiles portable C files (`json.c`, `printers.c`) with the host compiler for TDD. Use `#ifdef HOST_BUILD` to guard Mac-specific includes.

---

## Phase 1: Python Proxy (independent of Mac app)

### 1.1 Project setup
- `proxy/proxy.py` — entry point
- `proxy/config.py` — YAML config loader
- `proxy/models.py` — `PrinterStatus` dataclass (all fields from SPEC.md)
- `proxy/requirements.txt` — `pyyaml`, `httpx`, `pytest`
- `proxy/config.example.yaml`
- Tests first: `test_models.py` (serialization, defaults), `test_config.py` (YAML parsing, validation)

### 1.2 PrusaLink adapter (TDD)
`proxy/adapters/prusalink.py` — polls `/api/v1/status`, `/api/v1/job`, `/api/v1/info`. HTTP Digest auth via `httpx.DigestAuth`. State mapping per SPEC.md table. Tests use fixture JSON files.

### 1.3 Moonraker adapter (TDD)
`proxy/adapters/moonraker.py` — single call to `/printer/objects/query?print_stats&virtual_sdcard&extruder&heater_bed&display_status`. Optional `X-Api-Key`. Time remaining = `metadata.estimated_time - print_duration`. Tests use fixture JSON files.

### 1.4 Polling engine
`proxy/poller.py` — asyncio background task per printer, updates shared cache. Catches adapter exceptions → state "offline".

### 1.5 HTTP server (HTTP/1.0 compatible)
`proxy/server.py` — subclass `BaseHTTPRequestHandler` with `protocol_version = "HTTP/1.0"`. Two endpoints: `GET /printers` and `GET /printers/{id}`. Forces `Connection: close` and `Content-Length` headers. Serves from cache.

### 1.6 Docker packaging
`proxy/Dockerfile` — Python 3.12 slim, expose 8080.

### 1.7 End-to-end proxy test
Mock HTTP servers simulating PrusaLink/Moonraker responses. Start proxy, hit `/printers`, verify normalized output.

---

## Phase 2: MacTCPHelper Dependency (HIGH RISK — do early)

### 2.1 Clone and build MacTCPHelper
```bash
cd ~/Code
git clone https://github.com/antscode/MacTCPHelper.git
mkdir MacTCPHelper-build && cd MacTCPHelper-build
cmake ../MacTCPHelper \
  -DCMAKE_TOOLCHAIN_FILE=~/Code/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make && make install
```
**If this fails:** Inspect errors. Common issues: `bool` type conflicts (add `<stdbool.h>`), include path for bundled `MacTCP.h`. Fallback: write our own minimal `mactcp.h` + wrapper using `LaunchAPPL/Server/MacTCPStream.cc` patterns (PBControlSync calls).

### 2.2 Verify MacTCPHelper links
Minimal test app: call `InitNetwork()`, show result in an alert. Build, deploy to Basilisk II.

### 2.3 Verify SLiRP connectivity
Configure MacTCP in Basilisk II (static IP 10.0.2.15, gateway 10.0.2.2). Start proxy on host. Test `OpenConnection()` to 10.0.2.2:8080 from the Mac app. If this fails, debug with alert-based diagnostics showing error codes at each step.

---

## Phase 3: Data Model & JSON Parsing (host-testable, TDD)

### 3.1 Vendor jsmn
Download `jsmn.h` from `github.com/zserge/jsmn`, place at `src/jsmn.h`. Single header, zero allocation.

### 3.2 Printer data model (TDD)
`src/printers.h/.c` — fixed-size struct with all fields from SPEC.md. `PrinterList` holds up to 8 printers. Helper functions: `PrinterList_Init()`, `PrinterList_FindById()`, `FormatTimeRemaining()`. Host tests in `tests/test_printers.c`.

### 3.3 JSON parser (TDD)
`src/json.h/.c` — wraps jsmn for our specific schema. `ParsePrintersResponse()` and `ParseSinglePrinter()`. 256-token budget (~2KB). Host tests with literal JSON strings covering: multi-printer response, empty array, malformed JSON, missing fields, integer temp parsing.

---

## Phase 4: Mac App UI (hardcoded data, no networking)

### 4.1 Rez resources
`resources/PrintWatch.r` — MBAR, MENU (Apple/File/Options), WIND 128 (main window), STR# 128 (proxy config defaults), SIZE -1, ALRT 129 (error alert). Available Rez includes: `Multiverse.r`, `Menus.r`, `Windows.r`, `Dialogs.r`, `Processes.r` (at `~/Code/Retro68-build/toolchain/m68k-apple-macos/RIncludes/`).

### 4.2 Entry point and event loop
`src/main.c` — Toolbox init sequence (`InitGraf`, `InitFonts`, `InitWindows`, `InitMenus`, `TEInit`, `InitDialogs`, `InitCursor`), load MBAR, create window, `WaitNextEvent` loop. Populate `gPrinterList` with hardcoded test data for visual development.

### 4.3 Config reader
`src/config.h/.c` — `Config_Load()` reads STR# 128 via `GetIndString()`, converts Pascal→C strings.

### 4.4 List view
`src/ui.h/.c` — `UI_DrawListView()`. Each printer row: name (Geneva 12pt bold), state, progress bar (`FrameRect`/`PaintRect`), job filename, time remaining. Selection indicator with `►`. Use `InvalRect`/`BeginUpdate`/`EndUpdate` for flicker-free redraws.

### 4.5 Detail view
`UI_DrawDetailView()` — full-screen single printer: state, job, wide progress bar, time remaining, nozzle/bed temps. Back button.

### 4.6 Navigation
Arrow keys (list selection), Return (open detail), Escape (back to list), mouse clicks on rows. Row hit-testing by Y coordinate.

### 4.7 Menus
File: Refresh Now (⌘R), Quit (⌘Q). Options: Set Proxy Address…, Set Poll Interval… (dialogs deferred to Phase 6).

### 4.8 Visual polish in Basilisk II
Iterate on layout, font sizes, progress bar proportions, text truncation.

---

## Phase 5: Mac App Networking

### 5.1 HTTP client module
`src/network.h/.c` — wraps MacTCPHelper. `Net_Init()`, `Net_HttpGet()` (open connection → send GET → recv loop → close → extract body after `\r\n\r\n`). `Net_GiveTime()` callback calls `WaitNextEvent(everyEvent, &e, 1, NULL)` to stay responsive. Static 8KB TCP receive buffer, 4KB HTTP response buffer.

IP parsing: manual dotted-quad → `unsigned long` (don't use DNS, proxy is always an IP).

### 5.2 Poll cycle integration
`PollPrinters()` called when `TickCount() >= gNextPollTime`. Calls `Net_HttpGet("/printers")` → `ParsePrintersResponse()` → update `gPrinterList` → `InvalRect` to trigger redraw.

### 5.3 Startup check
`OpenDriver("\p.IPP", &refNum)` — if fails, show alert "MacTCP is not installed" and quit.

### 5.4 Status bar
Bottom 20px of window: "Last updated: HH:MM" or "Cannot reach proxy" or "Bad response".

### 5.5 End-to-end test
Proxy running on host → Basilisk II with MacTCP/SLiRP → Mac app displays live printer status.

---

## Phase 6: Polish & Hardening

### 6.1 Options dialogs
DLOG/DITL resources for proxy address and poll interval. `ModalDialog` + `ChangedResource`/`WriteResource` to update STR# 128.

### 6.2 Error recovery
Connection failures: wait full poll interval before retry. Partial responses: discard, show "Bad response". Stream creation failure: alert and disable networking.

### 6.3 Memory audit
Check `FreeMem()` at startup. Verify no leaks across poll cycles (all buffers are static/stack).

### 6.4 Real hardware test
Transfer to physical Mac SE, install MacTCP, point at proxy on LAN.

---

## Parallelism

- **Phase 1** (proxy) and **Phase 2** (MacTCPHelper) can run in parallel
- **Phase 3** (data model/JSON) can start immediately — no Mac dependencies
- **Phase 4** (UI) can start once Phase 3 is done — uses hardcoded data, no networking needed
- **Phase 5** (networking) requires both Phase 1 (proxy running) and Phase 2 (MacTCPHelper working)
- **Critical path:** Phase 0.2 → Phase 2.1 → 2.2 → 2.3 → Phase 5.1 → 5.5

## Verification

- **Proxy:** `pytest proxy/tests/` — unit tests for adapters, integration test with mock printers
- **Data model/JSON:** `make -C tests test` — host-compiled C tests
- **Mac app UI:** visual verification in Basilisk II with hardcoded data
- **End-to-end:** proxy on host + Mac app in Basilisk II displaying live status
- **Final:** real Mac SE hardware with MacTCP on physical network
