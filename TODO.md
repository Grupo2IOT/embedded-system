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
- [ ] **Add watchdog timer feed** — If any sensor library (e.g., DHT22) blocks or hangs, the irrigation loop stalls. Feed the ESP32 task watchdog in `loop()` and add a timeout recovery path.

## Calibration & Config

- [ ] **Externalize calibration constants** — `AIR_VALUE`, `WATER_VALUE`, `MAX_RESISTANCE`, and `MIN_RESISTANCE` are hardcoded in headers. Move them to a config struct or persistent storage (e.g., `Preferences` / NVS) so they can be tuned without recompiling.

## Documentation

- [x] **Sync class diagram with codebase** — `SensorBH1750`, `LecturaLuz`, and `APAGADO_EMERGENCIA` removed. Diagram now matches the actual firmware and hardware. `docs/hardware.md` created as formal BOM.
- [x] **README.md accuracy** — YL-69 description updated to reflect "resistivity proxy" rather than pure EC measurement. Second pump confirmed.
