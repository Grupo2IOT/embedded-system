# AquaEdge — Hardware Integration Guide

This document is the starting point for anyone taking the project from code to physical hardware. Follow the phases in order. Do not skip Phase 2 (real sensor verification) before adding simulation mode.

## Prerequisites

- Hardware listed in `docs/hardware.md` (BOM)
- PlatformIO installed (`pio` command available)
- USB cable and ESP32 board

## Phase 1: Dry Flash (No sensors attached)

Goal: confirm the build chain, board, and Serial connection work.

1. Build the project:
   ```bash
   pio run
   ```
2. Flash the board:
   ```bash
   pio run --target upload
   ```
3. Open the serial monitor:
   ```bash
   pio device monitor --baud 115200
   ```
4. Verify the `[SYSTEM READY]` message appears. If not, check the USB cable, port selection, or baud rate.

## Phase 2: Add One Real Sensor (HW-390 recommended)

Goal: understand what real data looks like before simulating anything.

**Why the HW-390 first?**
- It is the simplest: single analog pin, no external libraries, no pull-up resistors.
- It is the **primary source of truth** for soil moisture.

Steps:
1. Connect the HW-390 capacitive soil moisture sensor to **GPIO 32** (ADC1).
2. Flash the same firmware.
3. Open Serial telemetry and observe the raw ADC value.
4. Record the raw ADC value in:
   - **Dry air** → this will become your `AIR_VALUE` calibration constant.
   - **Fully submerged in water** → this will become your `WATER_VALUE` calibration constant.
5. Write the recorded values into `include/SoilMoistureSensor.h`.

> ⚠️ **Do not skip this phase.** If you add simulation mode before verifying real hardware, you risk validating your evaluator logic against fake data that does not match reality. Phase 2 is the foundation for everything that follows.

## Phase 3: Add Remaining Sensors (One by one)

Add the next sensor only after the previous one is verified and stable.

| Sensor | Pin | Prerequisites | Why add in this order |
|--------|-----|---------------|----------------------|
| DHT22 | GPIO 26 | Adafruit DHT library installed | No external resistors needed |
| DS18B20 | GPIO 25 | OneWire + DallasTemperature libraries, **4.7kΩ pull-up resistor** between data and 3.3V | Required for soil temperature; if you forget the pull-up you get -127°C |
| YL-69 | GPIO 34 | Raw ADC; no libraries | Measures resistivity (moisture + ions). Used as a **correlated proxy** alongside the HW-390. See the YL-69 note below. |
| Float switch (SB-3510LW) | GPIO 27 | Physical pull-down resistor (or `INPUT_PULLDOWN`) | Prevents floating pin when switch is open |

## Phase 4: Add Actuators

1. Connect the 2-channel relay module to **GPIO 14** (water pump) and **GPIO 13** (fertilizer pump).
2. Connect the pumps.
3. Run an end-to-end test:
   - Let the soil dry (or hold the HW-390 in air).
   - Verify the evaluator triggers irrigation.
   - Verify the water pump relay clicks and the pump runs.
   - Re-wet the sensor and verify the pump stops.

## Phase 5: Calibration

- `SoilMoistureSensor.h` (`AIR_VALUE`, `WATER_VALUE`): already calibrated in Phase 2.
- `SoilFertilitySensor.h` (`MAX_RESISTANCE`, `MIN_RESISTANCE`): measure in your actual soil. High-resistance = dry soil. Low-resistance = saturated soil.
- **Important**: The YL-69 cannot distinguish moisture from nutrients in isolation. The `SoilFertilitySensor` value only gets meaning when cross-checked against the HW-390 capacitive moisture reading. If both sensors agree, confidence is high. If they disagree, flag an anomaly.

## Phase 6: Add Simulation Mode (Native ESP32)

Goal: have a reliable demo fallback that mirrors the real data you observed in Phase 2.

**Why native ESP32 simulation?**
- The actuators (relays/pumps) must click during the demo. Backend-only simulation lives only on a screen.
- It validates the full embedded pipeline, not just the web UI.

### Architecture (Option A — Documented, Not Implemented)

Create `FakeSensor` classes inheriting from `BaseSensor` (e.g., `FakeSoilMoistureSensor`, `FakeDHT22Sensor`). In `main.cpp`, switch between real and fake sensors via a compile-time flag:

```cpp
#ifdef SIMULATION_MODE
    FakeSoilMoistureSensor moistureSensor;
    FakeDHT22Sensor dhtSensor;
    // ... other fake sensors
#else
    SoilMoistureSensor moistureSensor;
    DHT22Sensor dhtSensor;
    // ... other real sensors
#endif
```

**Why this approach is clean:**
- `IrrigationController` and `AgronomicEvaluator` remain untouched.
- No `#ifdef` scattered in business logic.
- The fake sensor is a dumb time-based generator: moisture decreases ~1% per tick, then rises when the pump is active.

### When to implement

- **After** Phase 2 (real sensor verified) and **after** Phase 5 (calibration done).
- The simulation must mimic the real ADC range and behavior observed during calibration.

## YL-69 Important Note

The `YL-69` / `FC-28` is a **resistive soil moisture sensor**, not a true electrical conductivity (EC) or fertility meter. It measures the resistance between two probes, which is dominated by soil moisture with a secondary contribution from dissolved ions (salinity/nutrients).

- It is used as a **correlated proxy** alongside the HW-390 capacitive sensor.
- When both sensors agree, confidence is high.
- When they disagree, it flags a potential anomaly (sensor failure, loose wire, or extremely low-ion wet soil).
- The "fertility" abstraction in `SoilFertilitySensor` is a pragmatic naming choice for the prototype, but the value only gets agronomic meaning when cross-checked with the capacitive moisture reading.

## Decision Log

| Decision | Rationale | Source |
|----------|-----------|--------|
| Verify real hardware before simulation | Prevents validating evaluator logic against fake data that doesn't match reality | `temp.md` analysis |
| Native ESP32 simulation > Backend simulation | Actuators must move; backend simulation is only a screen | `temp.md` analysis |
| `FakeSensor` class architecture | Keeps `IrrigationController` and `AgronomicEvaluator` untouched; clean separation | New proposal |
| `YL-69` reclassification | Acknowledged as resistive moisture proxy, not true EC sensor | Hardware review |

## Next Steps

1. Read `docs/hardware.md` for the full BOM and wiring rationale.
2. Read `README.md` for the build/run commands and architecture overview.
3. Read `TODO.md` for code-level improvements (averaging, debouncing, watchdog) that are not blockers for hardware integration.
4. Do **Phase 1** now. Confirm the board works.
