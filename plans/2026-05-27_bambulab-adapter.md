# Add Bambu Lab P1S adapter (LAN/developer mode)

## Context

PrintWatch currently supports PrusaLink and Moonraker printers via HTTP polling. Erik wants to add support for Bambu Lab P1S printers running in LAN/developer mode. This is architecturally different: Bambu uses MQTT over TLS (port 8883) with push-based delta updates, not HTTP request/response.

## Protocol summary

- **Transport**: MQTT 3.1.1 over TLS on port 8883 (self-signed cert, must skip verification)
- **Auth**: Username `bblp` (hardcoded), password is the printer's LAN Access Code (8 chars)
- **Topics**: `device/{serial}/report` (printer pushes status), `device/{serial}/request` (send commands)
- **Behavior**: P1S sends delta updates (only changed fields). Must send `pushall` command to get full state. Rate-limit `pushall` to no more than once per 5 minutes.
- **State field**: `gcode_state` — one of `IDLE`, `RUNNING`, `PAUSE`, `FAILED`, `FINISH`
- **Temps**: `nozzle_temper`, `nozzle_target_temper`, `bed_temper`, `bed_target_temper` (floats, Celsius)
- **Progress**: `mc_percent` (0-100 int), `mc_remaining_time` (minutes), `subtask_name` (job name)
- **Errors**: `print_error` (numeric code, 0 = none), `hms` array (health management alerts)

## Approach

### Fit MQTT into the polling architecture

The Bambu adapter manages a persistent MQTT connection internally. The poller still calls `poll()` on an interval like the other adapters, but instead of making an HTTP request, `poll()` returns the latest accumulated state from MQTT messages. This keeps the adapter interface consistent — no poller refactoring needed.

```
Poller._poll_loop()
  └─ bambulab.poll(client, cfg, connection)
       ├─ If not connected: establish MQTT, subscribe, send pushall
       ├─ Process any queued MQTT messages (delta merge)
       ├─ Periodic pushall every 5 minutes for full state refresh
       └─ Return PrinterStatus from merged state
```

The `BambuConnection` object is stored per-printer in the poller (like `_metadata_caches` for Moonraker), maintaining the MQTT client and accumulated state between poll cycles.

### Config

Add `serial` field to `PrinterConfig`. Reuse `password` for the LAN Access Code (the MQTT username is always "bblp", hardcoded in the adapter).

```yaml
printers:
  - id: "p1s"
    name: "Bambu P1S"
    type: "bambulab"
    model: "p1s"
    url: "192.168.1.70"        # printer IP (no http://, MQTT not HTTP)
    serial: "01P00A000000000"   # printer serial number
    password: "12345678"        # LAN Access Code from printer screen
```

### State mapping

| Bambu `gcode_state` | Normalized state |
|---------------------|-----------------|
| `IDLE`              | `idle`          |
| `RUNNING`           | `printing`      |
| `PAUSE`             | `paused`        |
| `FINISH`            | `finished`      |
| `FAILED`            | `error`         |

### Dependencies

Add `paho-mqtt>=2.0` to `pyproject.toml` dependencies (not dev — it's a runtime dependency for the proxy).

## File changes

### New files

- **`proxy/adapters/bambulab.py`** — The adapter module:
  - `BambuConnection` class: manages paho-mqtt client, TLS config, delta merging into accumulated `_state` dict, reconnection on disconnect
  - `poll(client, cfg, connection)` → `PrinterStatus`: checks connection, processes queued messages, returns status
  - `parse_report(data, cfg)` → `PrinterStatus`: pure parsing function for testing (no MQTT)
  - `BAMBU_STATE_MAP` dict for state normalization
  - Delta merging: deep-merge incoming `print` dict into `connection._state`
  - Periodic `pushall`: track last pushall time, re-send every 5 minutes

- **`proxy/tests/fixtures/bambulab_report_printing.json`** — Example MQTT report payload while printing
- **`proxy/tests/fixtures/bambulab_report_idle.json`** — Example MQTT report payload while idle

### Modified files

- **`proxy/config.py`**:
  - Add `serial: str = ""` to `PrinterConfig`
  - Add `"bambulab"` to the allowed types validation

- **`proxy/poller.py`**:
  - Add `from .adapters import bambulab`
  - Add `_bambu_connections: dict[str, bambulab.BambuConnection]` in `__init__`
  - Initialize a `BambuConnection` for each bambulab printer
  - Add dispatch case: `elif cfg.type == "bambulab": status = await bambulab.poll(client, cfg, self._bambu_connections[cfg.id])`
  - In `stop()`: disconnect all Bambu connections

- **`pyproject.toml`**: Add `paho-mqtt>=2.0` to `dependencies`

- **`proxy/config.example.yaml`**: Add commented Bambu example printer entry

- **`proxy/tests/test_adapters.py`**: Add `TestBambuLab` class with:
  - `test_printing_state`, `test_idle_state`, `test_all_state_mappings`
  - `test_temps_are_integers` (Bambu sends floats, adapter must int())
  - `test_time_remaining_in_seconds` (Bambu sends minutes, adapter converts to seconds)
  - `test_error_state`
  - `test_delta_merge` (merge partial update into existing state)

## MQTT threading considerations

paho-mqtt uses a background thread for network I/O (`loop_start()`). The `poll()` function runs in an asyncio context. To bridge:
- paho-mqtt callbacks (`on_message`) write to a thread-safe queue (`queue.Queue`)
- `poll()` drains the queue synchronously (fast, non-blocking — just dict merges)
- No asyncio/threading conflicts since the queue is the only shared state

## Verification

1. **Unit tests**: `uv run pytest proxy/tests/test_adapters.py::TestBambuLab -v`
   - Test parsing with fixture data
   - Test all state mappings
   - Test delta merging
   - Test time conversion (minutes → seconds)
2. **Integration test**: Run the proxy with a Bambu P1S on the LAN
   - `uv run printwatch-proxy proxy/config.yaml`
   - `curl http://localhost:8080/printers` — verify Bambu printer appears with correct state/temps
3. **End-to-end**: View in Basilisk II, click printer detail, verify status and image display

## TDD sequence

1. Write `test_idle_state` and `test_printing_state` with fixture data → fail
2. Implement `parse_report()` and `BAMBU_STATE_MAP` → pass
3. Write `test_delta_merge` → fail
4. Implement `BambuConnection._merge_state()` → pass
5. Write remaining tests (temps, time conversion, errors) → implement as needed
6. Wire up config validation and poller dispatch
7. Integration test with real printer
