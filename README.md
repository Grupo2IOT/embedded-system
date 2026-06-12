# AquaEdge

AquaEdge is an ESP32-based smart irrigation controller that monitors soil and environment conditions, evaluates them against agronomic thresholds, and automatically controls water and fertilizer pumps. It is built with the Arduino framework on ESP32 and managed with PlatformIO.

## What it does

- Reads soil moisture, soil fertility (EC), soil temperature, air temperature/humidity, and water tank level.
- Evaluates the crop state every 5 seconds.
- Automatically turns the water and/or fertilizer pumps on or off based on thresholds.
- Prevents pumps from running if the water tank is empty (hard safety short-circuit).
- Streams telemetry readings and diagnoses over Serial at 115200 baud.

## Hardware

| Component | Pin | Role |
|-----------|-----|------|
| Capacitive soil moisture sensor | GPIO 32 (ADC1) | Soil moisture % |
| Resistive soil fertility sensor (YL-69) | GPIO 34 (ADC1) | Electrical conductivity proxy |
| DS18B20 soil temperature probe | GPIO 25 (OneWire) | Soil temperature |
| DHT22 | GPIO 26 | Air temperature & humidity |
| Water level float switch | GPIO 27 | Tank level (digital) |
| Water pump relay | GPIO 12 | Water pump on/off |
| Fertilizer pump relay | GPIO 14 | Fertilizer pump on/off |

Board: **ESP32-DevKit** (see `platformio.ini` for exact environment).

## Architecture

The firmware is split into a 4-layer pipeline:

1. **Sensors** (`src/*Sensor.cpp`) read hardware and return typed DTOs defined in `include/Readings.h`.
2. **AgronomicEvaluator** (`src/AgronomicEvaluator.cpp`) receives the full `CropState` and produces an `AgronomicDiagnosis`.
3. **Actuators** (`src/WaterPump.cpp`, `src/FertilizerPump.cpp`) execute `Command` values from the diagnosis.
4. **TelemetryClient** (`src/TelemetryClient.cpp`) prints the consolidated state and diagnosis to Serial.

`IrrigationController` (`src/IrrigationController.cpp`) orchestrates the pipeline on a non-blocking 5-second tick using `millis()`. The `main.cpp` entry point wires all GPIO pins and starts the loop.

A PlantUML class diagram is available in `docs/class-diagram.puml`.

## Quick start

Requirements:
- [PlatformIO Core](https://platformio.org/install/cli) (or the PlatformIO IDE extension for VS Code)
- ESP32-DevKit board
- USB cable to connect the board

Build:
```bash
pio run
```

Flash:
```bash
pio run --target upload
```

Open the serial monitor to see telemetry:
```bash
pio device monitor --baud 115200
```

Clean build artifacts:
```bash
pio run --target clean
```

## Calibration

The ADC-based sensors (soil moisture and fertility) use device-specific constants stored in their headers. You must calibrate them against your actual hardware and soil:

- **SoilMoistureSensor.h** (`AIR_VALUE`, `WATER_VALUE`): measure raw ADC values in dry air and fully submerged in water.
- **SoilFertilitySensor.h** (`MAX_RESISTANCE`, `MIN_RESISTANCE`): measure raw ADC values in distilled water (low nutrients) and nutrient-rich water (high nutrients).

## Safety

If the water tank is detected as empty, the evaluator immediately disables both pumps and short-circuits the evaluation loop. This behavior is mandatory and must be preserved by any modifications.

## Project structure

```
├── include/          # Header files (DTOs, base classes, interfaces)
├── src/              # Implementation files and main.cpp entry point
├── docs/             # PlantUML class diagram
├── test/             # PlatformIO unit tests (currently empty)
├── platformio.ini    # PlatformIO project configuration
└── AGENTS.md         # Guidelines for coding agents (automation context)
```

## Dependencies

Managed by PlatformIO and declared in `platformio.ini`:
- `paulstoffregen/OneWire@^2.3.8`
- `milesburton/DallasTemperature@^4.0.6`
- `adafruit/DHT sensor library@^1.4.7`

## Adding new sensors or actuators

- **Sensor**: inherit from `BaseSensor`, add a reading struct to `include/Readings.h`, implement `read()` returning the DTO, and compose it into `IrrigationController`.
- **Actuator**: inherit from `BaseActuator`, add a command to the `Command` enum in `include/Types.h`, implement `execute()`, and compose it into `IrrigationController`.

## Notes

- The main loop is fully non-blocking. Do not add `delay()` calls that could stall the irrigation tick.
- GPIO mapping is centralized in `src/main.cpp`; do not scatter pin numbers across classes.
- All identifiers are in English. Some older comments are in Spanish; new code should be fully in English.
- WiFi credentials, API keys, or any secrets must be kept in `.env` or `secrets.h` (both are `.gitignore`d) and never committed.

## License

MIT (or add your license here)
