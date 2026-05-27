# PrintWatch

A 3D printer monitoring app for the Macintosh SE, built for System 7. A Python proxy polls modern printer APIs (PrusaLink, Moonraker) and serves normalized status over HTTP/1.0 for the classic Mac client.

## Architecture

```
┌──────────────┐        ┌───────────────┐        ┌──────────────┐
│  Mac SE App  │──HTTP──│  Proxy Server │──poll──│  3D Printers │
│  (System 7)  │  1.0   │  (Python)     │        │  (LAN)       │
└──────────────┘        └───────────────┘        └──────────────┘
```

## Proxy Setup

### Prerequisites

- [uv](https://docs.astral.sh/uv/getting-started/installation/) (Python package manager)

### Install and Run

```bash
uv sync

cp proxy/config.example.yaml config.yaml
# Edit config.yaml with your printer addresses and credentials

uv run printwatch-proxy config.yaml
```

The proxy starts on port 8080 by default and polls your printers at the configured interval.

### Configuration

Copy `proxy/config.example.yaml` and edit it:

```yaml
proxy:
  host: "0.0.0.0"
  port: 8080
  poll_interval: 10    # seconds between polls

printers:
  - id: "mk4"
    name: "Prusa MK4"
    type: "prusalink"   # or "moonraker"
    model: "MK4S"       # shown in UI after printer name
    url: "http://192.168.1.50"
    username: "maker"
    password: "your-api-key-here"
```

### API

- `GET /printers` — all printer statuses as JSON
- `GET /printers/{id}` — single printer status

### Tests

```bash
uv run pytest proxy/tests/
```

## Mac SE Client

The client is a System 7 application built with the Retro68 cross-compiler toolchain.

### Prerequisites

```bash
brew install cmake gmp mpfr libmpc boost bison flex texinfo
```

### Build Retro68 (one-time, ~30-60 min)

```bash
git clone --recursive https://github.com/autc04/Retro68.git ~/Code/Retro68
mkdir ~/Code/Retro68-build && cd ~/Code/Retro68-build
../Retro68/build-toolchain.bash --no-ppc --clean-after-build
```

### Build the App

```bash
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=~/Code/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake
make
```

### Test in Basilisk II

```bash
cp build/PrintWatch.bin ~/Code/Basilisk\ II/shared/
```

The file appears on the emulated Mac's desktop as a mounted volume.

## Toolchain

| Component | Location |
|-----------|----------|
| Retro68 source | `~/Code/Retro68` |
| Build output / toolchain | `~/Code/Retro68-build/toolchain/` |
| CMake toolchain file | `~/Code/Retro68-build/toolchain/m68k-apple-macos/cmake/retro68.toolchain.cmake` |
| Basilisk II | `~/Code/Basilisk II/` |

## Documentation

See `docs/` for detailed guides:

- **[RETRO68_SETUP.md](docs/RETRO68_SETUP.md)** — Toolchain reference and troubleshooting
- **[EMULATOR_SETUP.md](docs/EMULATOR_SETUP.md)** — Basilisk II and Mini vMac configuration
- **[WORKFLOW.md](docs/WORKFLOW.md)** — Dev workflow with Claude Code
