# AquaEdge — Hardware Bill of Materials (BOM)

This document lists every physical component used by the AquaEdge firmware.

## Controller

| SKU | Description | Role | Notes |
|-----|-------------|------|-------|
| ESP32-WIFI | ESP32 DEVKITV1 30PINES | Main controller | Arduino framework via PlatformIO |

## Sensors

| SKU | Description | Code class | Pin | What it measures | Truthiness |
|-----|-------------|------------|-----|------------------|------------|
| SEN-0193 | HW-390 Capacitive Soil Moisture | `SoilMoistureSensor` | GPIO 32 (ADC1) | Soil moisture % | **Primary source of truth** |
| YL-69 | FC-28 Resistive Soil Sensor | `SoilFertilitySensor` | GPIO 34 (ADC1) | Resistance proxy (moisture + ions) | **Secondary / correlated** |
| SEN-DS18B20 | DS18B20 Waterproof Probe 1m | `SoilTemperatureSensor` | GPIO 25 (OneWire) | Soil temperature | Primary |
| DHT22-PCB | DHT22 Air Temp/Humidity | `DHT22Sensor` | GPIO 26 | Air temperature & humidity | Primary |
| SB-3510LW | Float Switch (Liquid Level) | `WaterLevelSensor` | GPIO 27 | Water tank empty/sufficient | Primary |

### Sensor Notes

- **YL-69 is NOT a true EC/fertility meter.** It measures resistance between two probes, which is dominated by soil moisture with a secondary contribution from dissolved ions (salinity/nutrients). It is used as a *correlated proxy* alongside the HW-390 capacitive sensor. When both agree, confidence is high. When they disagree, it flags a potential anomaly (sensor failure or extremely low-ion wet soil).
- **ADC calibration required** for both soil sensors. See `README.md` Calibration section.
- **Raw ADC values are printed in telemetry** for both ADC sensors (`SoilMoistureReading.rawValue` and `SoilFertilityReading.rawValue`). Use this to calibrate `AIR_VALUE`/`WATER_VALUE` (HW-390) and `MAX_RESISTANCE`/`MIN_RESISTANCE` (YL-69) without needing a separate test sketch.
- **Calibration values**: `AIR_VALUE=3120` / `WATER_VALUE=1070` (HW-390); `MAX_RESISTANCE=4095` / `MIN_RESISTANCE=1400` (YL-69 with buffer for fertilizer).

## Actuators

| SKU | Description | Code class | Pin | Role |
|-----|-------------|------------|-----|------|
| ARD-RE2 | 2-Channel 5V Relay Module (active-LOW) | `WaterPump` + `FertilizerPump` | GPIO 14 (water) / GPIO 13 (fertilizer) | Switches pump power |
| RS-SUMERGIBLE | Mini Submersible Pump 3V~6V (x2) | — | — | Pump 1 = Water, Pump 2 = Fertilizer/Nutrients |

### Actuator Wiring (Read This Carefully)

**Relay module → ESP32 (signal & logic power):**
| Relay Pin | Connect To |
|-----------|------------|
| VCC | ESP32 **5V** (powers the relay logic/LEDs, ~20mA) |
| GND | ESP32 **GND** |
| IN1 | **GPIO 14** |
| IN2 | **GPIO 13** |

> **JD-VCC jumper**: The yellow cap on the side of the module should be **ON** (covering the JD-VCC and VCC pins). This is the default and simplest configuration. Do not remove it unless you know what you're doing.

**Relay module → Pumps (switched power):**
The relay acts as a **switch** in the pump's power line. The pumps get power from your **external battery + step-down supply**, NOT from the ESP32.

```
Battery + (5V) ───┬────────────────────── Pump (+)
                  │
Battery - (GND) ──┼── Relay COM (Common)
                  │
                  └── Relay NO (Normally Open) ─── Pump (-)
```

Repeat for the second pump on the second relay channel.

**Active-LOW logic:**
- `digitalWrite(pin, LOW)` → relay energizes → pump **ON**
- `digitalWrite(pin, HIGH)` → relay de-energizes → pump **OFF**

The firmware (`WaterPump.cpp`, `FertilizerPump.cpp`) handles this automatically.

### Actuator Warnings

- GPIO 14 and 13 are safe digital outputs. The previous GPIO 12 (strapping pin) assignment has been corrected. See `TODO.md`.
- **The ESP32 does NOT power the pumps.** Pumps draw too much current and will brown-out the ESP32. Always use a separate power supply (e.g., lithium batteries + step-down to 5V) for the pumps.
- **The relay module VCC goes to ESP32 5V** (not 3.3V). The relay logic circuitry needs 5V to reliably trigger. The current draw is small (~20mA per channel) and safe for the ESP32's 5V regulator.

## Not in this prototype

The following sensors were proposed in earlier design iterations but are **not present** in the current hardware or firmware:

- BH1750 (I2C light sensor) — deferred to a future revision

## Wiring Notes

These notes explain why specific pins were chosen and what physical requirements exist on each connection:

- **GPIO 32 (ADC1_CH4) → HW-390**: ADC1 channel. This channel is immune to Wi-Fi interference, making it ideal for clean analog readings from the capacitive moisture sensor.
- **GPIO 34 (ADC1_CH6) → YL-69**: Input-only pin (no internal pull-up). This is ideal for capturing the raw voltage from the YL-69 amplifier without internal bias affecting the reading.
- **GPIO 26 → DHT22**: Standard digital pin, stable for the Adafruit single-wire protocol.
- **GPIO 25 → DS18B20**: OneWire bus. Requires an **external 4.7kΩ pull-up resistor** between the data line and 3.3V. Without this, the Dallas Temperature library returns `-127°C` (no device detected).
- **GPIO 27 → Float switch (SB-3510LW)**: Simple digital input. Configure with a **physical pull-down resistor** (or `INPUT_PULLDOWN` in software) to prevent the pin from floating when the switch is open.
- **GPIO 14 → Water pump relay (IN1)**: Safe digital output. Not a strapping pin; does not interfere with ESP32 boot. Signal is **active-LOW**.
- **GPIO 13 → Fertilizer pump relay (IN2)**: Safe digital output. Not a strapping pin. Signal is **active-LOW**.

## Full wiring reference

See `src/main.cpp` for the canonical GPIO pin assignments.
