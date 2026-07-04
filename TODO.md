# AquaEdge — TODO / Technical Debt

This file tracks known issues and planned improvements before hardware deployment.

**For the step-by-step hardware integration guide, see `docs/GETTING_STARTED.md`.** This file tracks code-level improvements and technical debt.

## Hardware & Wiring

- [x] **Move water pump relay off GPIO 12** — Water pump moved to GPIO 14, fertilizer pump to GPIO 13. Both are safe digital outputs. Reviewed in `temp.md`.
- [x] **Confirm fertilizer pump hardware** — Confirmed: 2 pumps exist (water + fertilizer). Updated in `list-items.md` and `docs/hardware.md`.
- [x] **YL-69 sensor intent** — DECIDED: YL-69 measures resistance (moisture + ions). It is used as a **correlated proxy** alongside the HW-390 capacitive sensor. The "fertility" abstraction is a pragmatic naming choice for the prototype, but the value gets meaning only when cross-checked with the capacitive moisture reading. See `docs/hardware.md` for the full rationale.

## Signal Quality & Robustness

- [ ] **Add ADC multi-sample averaging** — `SoilMoistureSensor` and `SoilFertilitySensor` take a single `analogRead()`. Add 10–20 sample averaging with outlier rejection to reduce noise and false `isValid = false` triggers.
- [ ] **Add float switch debouncing** — `WaterLevelSensor` reads the digital pin once. Mechanical float switches bounce. Require the pin to be stable for 100–200 ms before declaring a state change.
- [x] **Fix Phase 1 dry-flash boot loop** — ESP32 reset repeatedly when DS18B20 was missing. Fixed by detecting `getDeviceCount() == 0` in `SoilTemperatureSensor::begin()` and skipping `requestTemperatures()` in `read()`.
- [x] **Fix floating water-level pin** — Changed `WaterLevelSensor::begin()` from `INPUT` to `INPUT_PULLDOWN` to prevent false state changes when the float switch is open.
- [x] **Fix serial monitor reset loop** — Added `monitor_dtr = 0` and `monitor_rts = 0` to `platformio.ini` to prevent `pio device monitor` from toggling the ESP32 reset lines after upload.
- [x] **Add watchdog timer feed** — `yield()` added between each `begin()` call in `IrrigationController`. `SoilTemperatureSensor` now detects missing DS18B20 and skips blocking `requestTemperatures()` to avoid 750ms watchdog stalls.
- [ ] **Add watchdog timer feed in `loop()`** — If a sensor library hangs during the main `tick()`, the loop still stalls. Add `yield()` inside `tick()` or move sensor reads to a FreeRTOS task.
- [x] **Add raw ADC value to moisture telemetry** — `SoilMoistureReading` and `SoilFertilityReading` now include `rawValue` (0–4095) which is printed in telemetry packets. This helps calibration without requiring temporary debug prints.

## Calibration & Config

- [x] **Calibrate HW-390** — `AIR_VALUE=3120` (dry air), `WATER_VALUE=1070` (room-temp water). Verified in telemetry.
- [x] **Calibrate YL-69** — `MAX_RESISTANCE=4095` (dry air), `MIN_RESISTANCE=1400` (tap water with 230-point buffer for fertilizer). Verified in telemetry.
- [ ] **Externalize calibration constants** — `AIR_VALUE`, `WATER_VALUE`, `MAX_RESISTANCE`, and `MIN_RESISTANCE` are hardcoded in headers. Move them to a config struct or persistent storage (e.g., `Preferences` / NVS) so they can be tuned without recompiling.

## Edge Gateway & Connectivity

- [x] **Add WiFi + HTTPClient to TelemetryClient** — `TelemetryClient` now connects to WiFi, builds nested JSON with ArduinoJson, and POSTs to `EDGE_GATEWAY_URL`. Failure is fire-and-forget; the control loop never blocks.
- [x] **Create `secrets.h` template** — Added `secrets.h.example` with WiFi creds, edge URL, API key, and device ID. Documented in README.
- [x] **Add non-blocking WiFi reconnection** — `begin()` attempts connection with 10s timeout. If it fails, `_ensureWiFi()` retries every 30s in the background. `tick()` is never stalled.
- [x] **Edge gateway: bidirectional command channel (Phase 2)** — Implemented **piggybacked commands** instead of polling. The edge returns `200 OK` with `{"commands": [...]}` in the telemetry POST response when commands are queued; otherwise `204 No Content`. ESP32 parses response, enforces Tier 1 safety (tank empty → reject), executes fixed-duration overrides, and reports results in next telemetry payload. See `docs/edge_architecture.md` Section 11.

## Documentation

- [x] **Sync class diagram with codebase** — `SensorBH1750`, `LecturaLuz`, and `APAGADO_EMERGENCIA` removed. Diagram now matches the actual firmware and hardware. `docs/hardware.md` created as formal BOM.
- [x] **README.md accuracy** — YL-69 description updated to reflect "resistivity proxy" rather than pure EC measurement. Second pump confirmed.
- [x] **Edge architecture document** — Created `docs/edge_architecture.md` with JSON schema, HTTP contract, SQLite schema, tiered authority model, TimescaleDB future note, and Phase 2 command stub.
