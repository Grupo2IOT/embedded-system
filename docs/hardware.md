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

## Actuators

| SKU | Description | Code class | Pin | Role |
|-----|-------------|------------|-----|------|
| ARD-RE2 | 2-Channel 5V Relay Module | `WaterPump` + `FertilizerPump` | GPIO 14 (water) / GPIO 13 (fertilizer) | Switches pump power |
| RS-SUMERGIBLE | Mini Submersible Pump 3V~6V (x2) | — | — | Pump 1 = Water, Pump 2 = Fertilizer/Nutrients |

### Actuator Warnings

- GPIO 14 and 13 are safe digital outputs. The previous GPIO 12 (strapping pin) assignment has been corrected. See `TODO.md`.
- The relay module should be powered by an external 5V supply, not the ESP32 3.3V rail, if driving pumps directly.

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
- **GPIO 14 → Water pump relay**: Safe digital output. Not a strapping pin; does not interfere with ESP32 boot.
- **GPIO 13 → Fertilizer pump relay**: Safe digital output. Not a strapping pin.

## Full wiring reference

See `src/main.cpp` for the canonical GPIO pin assignments.
