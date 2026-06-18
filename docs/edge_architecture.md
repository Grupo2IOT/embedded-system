# AquaEdge — Edge Gateway Architecture

> **Scope**: This document defines the contract between the ESP32 embedded firmware and the Python/Flask edge gateway. The edge gateway itself lives in a **separate repository**; this document exists so the embedded team and the edge team share a single source of truth.

## 1. Overview

| Layer | Technology | Role |
|-------|------------|------|
| **Device** | ESP32 (Arduino framework) | Real-time control, safety, local evaluator, telemetry emitter |
| **Transport** | WiFi + HTTP POST | Device pushes telemetry; future Phase 2 adds command polling |
| **Edge Gateway** | Python + Flask (separate repo) | Reception, validation, SQLite persistence, alerting, dashboard |
| **Future Cloud** | TBD (REST or MQTT broker) | Long-term analytics, multi-device fleet management |

**Design philosophy**: The ESP32 is the **safety authority**. The edge gateway is the **intelligence and history layer**. The edge can suggest, override, and configure — but the device can refuse unsafe commands.

---

## 2. Network Topology

```
+-----------+       WiFi (same LAN)       +------------------+
|  ESP32    |  ----------------------------> |  Laptop / RPi   |
| (Device)  |      POST /api/v1/telemetry   |  (Flask Edge)   |
|           |                               |  SQLite DB      |
+-----------+                               +------------------+
                                                    |
                                                    | Future
                                                    v
                                             +------------------+
                                             |  Cloud Backend   |
                                             |  (TimescaleDB?)  |
                                             +------------------+
```

- ESP32 and edge gateway are on the **same WiFi network** (home router / lab AP).
- Edge gateway URL is configured in `secrets.h` on the ESP32 (e.g., `http://192.168.1.100:5000`).
- **No Serial gateway** — Serial at 115200 baud is used for local debugging only.

---

## 3. JSON Payload Schema (Device → Edge)

The ESP32 sends a single JSON object via HTTP POST. It includes **raw values**, **validity flags**, **local diagnosis**, and **actuator state**.

```json
{
  "meta": {
    "device_id": "aquaedge-01",
    "firmware_version": "1.0.0",
    "tick_count": 1247,
    "timestamp_utc": "2025-06-18T14:32:01Z",
    "wifi_rssi_dbm": -62
  },
  "sensors": {
    "soil_moisture": {
      "value": 45.2,
      "unit": "%",
      "raw_adc": 2048,
      "is_valid": true
    },
    "soil_fertility": {
      "value": 3.8,
      "unit": "mS/cm",
      "raw_adc": 1876,
      "is_valid": true
    },
    "soil_temperature": {
      "value": 22.5,
      "unit": "C",
      "is_valid": true
    },
    "air": {
      "temperature": 24.1,
      "humidity": 67.0,
      "is_valid": true
    },
    "water_level": {
      "status": "SUFFICIENT",
      "is_valid": true
    }
  },
  "diagnosis": {
    "needs_irrigation": false,
    "needs_fertilization": false,
    "alert_message": null
  },
  "actuators": {
    "water_pump": "OFF",
    "fertilizer_pump": "OFF"
  },
  "system_health": {
    "overall": "HEALTHY",
    "failed_sensors": [],
    "pending_commands": []
  }
}
```

### Field rationale

| Field | Why it's included |
|-------|-------------------|
| `raw_adc` | Calibration and anomaly detection. If the edge sees `raw_adc=4095` while `value=100%`, it knows the sensor is pegged. |
| `is_valid` | Every sensor DTO already has this. The edge must know if a sensor failed so it doesn't trigger alerts on bad data. |
| `diagnosis` | The device's local evaluator opinion. The edge compares this with its own trend analysis. If they disagree, flag it. |
| `actuators` | Ground truth of what the device is *actually doing*. The edge can detect "commanded ON but still OFF" mismatches. |
| `wifi_rssi_dbm` | Network health. Sudden drops predict telemetry gaps. |
| `tick_count` | Monotonic counter. Detects reboots, missed ticks, or duplicate packets. |

---

## 4. HTTP Contract

### `POST /api/v1/telemetry`

**Request headers:**
```http
Content-Type: application/json
X-API-Key: <from secrets.h>
```

**Request body:** JSON payload (schema above).

**Edge responses:**

| Status | Meaning | ESP32 behavior |
|--------|---------|----------------|
| `204 No Content` | Success, no commands pending | Continue normal loop |
| `200 OK` + body | Success, commands included (Phase 2) | Parse and queue commands |
| `400 Bad Request` | JSON malformed or validation failed | Log error, increment `tx_failures`, continue |
| `401 Unauthorized` | API key mismatch | Log error, continue |
| `500 Server Error` | Edge crashed | Log error, continue |
| Timeout / no connection | Network down | Log error, continue. **Do not block.** |

**ESP32 transport behavior:**
- Fire-and-forget. The control loop (sensors → evaluator → actuators) **never waits** for HTTP.
- If WiFi is disconnected, skip the POST and continue the local loop.
- WiFi reconnection is attempted in the background (non-blocking).
- `tx_failures` counter is included in Serial debug logs for local diagnostics.

---

## 5. SQLite Schema (Normalized)

The edge gateway stores each telemetry packet as a **single row** in a normalized table. This makes time-series queries fast and simple.

```sql
CREATE TABLE telemetry (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    received_at     DATETIME DEFAULT CURRENT_TIMESTAMP,
    device_id       TEXT NOT NULL,
    firmware_version TEXT,
    tick_count      INTEGER NOT NULL,
    timestamp_utc   TEXT NOT NULL,
    wifi_rssi_dbm   INTEGER,

    -- Sensors
    soil_moisture_value     REAL,
    soil_moisture_raw_adc   INTEGER,
    soil_moisture_is_valid  BOOLEAN,

    soil_fertility_value    REAL,
    soil_fertility_raw_adc  INTEGER,
    soil_fertility_is_valid BOOLEAN,

    soil_temp_value         REAL,
    soil_temp_is_valid      BOOLEAN,

    air_temp_value          REAL,
    air_humidity_value      REAL,
    air_is_valid            BOOLEAN,

    water_level_status      TEXT,
    water_level_is_valid    BOOLEAN,

    -- Diagnosis
    needs_irrigation        BOOLEAN,
    needs_fertilization     BOOLEAN,
    alert_message           TEXT,

    -- Actuators
    water_pump_state        TEXT,
    fertilizer_pump_state   TEXT,

    -- Health
    system_health_overall   TEXT,
    failed_sensors_json     TEXT  -- JSON array, e.g. ["soil_temperature"]
);

-- Indexes for fast queries
CREATE INDEX idx_telemetry_device_time ON telemetry(device_id, timestamp_utc);
CREATE INDEX idx_telemetry_received     ON telemetry(received_at);
```

**Why normalized over JSON blobs?**
- Easy SQL queries: `SELECT soil_moisture_value FROM telemetry WHERE device_id = 'aquaedge-01' AND timestamp_utc > '2025-06-17'`
- Easy aggregation: `SELECT AVG(soil_moisture_value) FROM telemetry WHERE ...`
- Easy alerting: `SELECT * FROM telemetry WHERE water_level_status = 'EMPTY'`

---

## 6. Tiered Authority Model

Who decides what? A clear hierarchy prevents conflicts and keeps the system safe.

### Tier 1: Safety Rules (ESP32 — absolute veto)
- Water tank is `EMPTY` → **no pump runs, ever**, regardless of edge or user.
- Sensor hardware failure (`isValid = false`) → evaluator may refuse to act on that sensor.
- Future: pump thermal overload → no pump.

> **Rule**: The edge can *ask*, but the device is physically present. It has the final say on safety.

### Tier 2: Evaluator Rules (ESP32 — default optimization)
- Moisture < 30% → irrigate.
- Fertility < 2.5 mS/cm → fertilize.
- These thresholds are hardcoded in `AgronomicEvaluator` and run locally every tick.

> **Rule**: These are the "autopilot" rules. They keep the plant alive when the edge is offline.

### Tier 3: Edge / User Override (Edge gateway — can override Tier 2)
- User clicks "Force Water Now for 30s" on the dashboard.
- Edge sends command: `{"override": {"water_pump": "ON", "duration_sec": 30}}`.
- ESP32 receives it → checks Tier 1 (tank not empty? safe?) → **obeys**, bypassing Tier 2 evaluator.

> **Rule**: The user (via edge) can override the autopilot, but **never** the safety interlocks.

### Command Result Reporting (Phase 2)
If the edge sends a command and the ESP32 rejects it (Tier 1 violation), the next telemetry packet includes:

```json
"system_health": {
  "pending_commands": [
    {
      "command": "TURN_ON_WATER",
      "executed": false,
      "reason": "SAFETY_VIOLATION: tank_empty"
    }
  ]
}
```

This closes the loop: the user sees *why* their command was ignored.

---

## 7. ESP32 Responsibilities

1. **Maintain WiFi connection** — connect on boot, reconnect in background if lost.
2. **Run the control loop every 5 seconds** — sensors → evaluator → actuators. **Never block** for network.
3. **Serialize telemetry** — JSON payload with all fields defined in Section 3.
4. **POST to edge** — fire-and-forget. Failure does not stall the loop.
5. **Enforce Tier 1 safety** — refuse any command that violates physical safety.
6. **Expose debug info on Serial** — WiFi status, HTTP errors, `tx_failures` counter.

### Secrets management (`secrets.h`)
```cpp
#ifndef SECRETS_H
#define SECRETS_H

const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASSWORD = "your-password";
const char* EDGE_GATEWAY_URL = "http://192.168.1.100:5000/api/v1/telemetry";
const char* API_KEY = "your-secret-key";

#endif
```

`secrets.h` is in `.gitignore` and never committed.

---

## 8. Edge Gateway Responsibilities

1. **Receive telemetry** — validate JSON schema, reject malformed packets (400).
2. **Persist to SQLite** — insert into normalized `telemetry` table.
3. **Serve dashboard** — Flask routes to query recent data, render charts.
4. **Alerting** — detect anomalies (e.g., `tank_empty`, `failed_sensors`, stale data).
5. **Issue commands** — Phase 2: accept user overrides, queue commands for device polling.
6. **Forward to cloud** — Future: batch-upload to cloud backend.

---

## 9. Security

| Concern | Mitigation |
|---------|------------|
| WiFi credentials | Stored in `secrets.h`, gitignored |
| API authentication | `X-API-Key` header on every POST |
| Local network exposure | Edge gateway binds to LAN IP only (not 0.0.0.0) |
| Replay attacks | `tick_count` + `timestamp_utc` allow edge to reject old packets |
| No TLS on local network | Acceptable for lab/demo. For production, add HTTPS or MQTT over TLS. |

---

## 10. Future: TimescaleDB (Idea — Not Committed)

**What it is**: A PostgreSQL extension optimized for time-series data (IoT metrics, monitoring).

### Advantages
- **Automatic time partitioning** — queries on "last 24h" stay fast even with billions of rows.
- **Compression** — old data is compressed 90%+, saving disk.
- **SQL-native** — same queries as SQLite; migration is mostly a connection-string change.
- **Built-in aggregation** — `time_bucket()`, `candlestick()`, continuous aggregates.
- **Ecosystem** — works with Grafana, Pandas, SQLAlchemy.

### Disadvantages
- **Heavier than SQLite** — requires a running Postgres server (not a single file).
- **Overkill for one device** — TimescaleDB shines at 10+ devices or high-frequency data.
- **Operational cost** — backups, updates, memory usage.

### Verdict
- **Now**: SQLite is perfect for the laptop edge gateway (single device, local, zero ops).
- **Future**: If the fleet grows beyond 5 devices or you need real-time analytics, **SQLite → TimescaleDB migration is straightforward** because both speak SQL. The Flask app would swap the SQLAlchemy connection string. No schema rewrite needed.

---

## 11. Future: Bidirectional Commands (Phase 2 — Not Yet Implemented)

**Goal**: Allow the edge / user to send commands and config updates to the ESP32.

**Transport**: HTTP polling. ESP32 adds a `GET /api/v1/commands` request every N ticks (e.g., every 30 seconds, or piggybacked on telemetry POST responses).

**Example edge response (200 OK body):**
```json
{
  "commands": [
    {
      "id": "cmd-42",
      "type": "OVERRIDE_PUMP",
      "target": "water_pump",
      "state": "ON",
      "duration_sec": 30,
      "issued_by": "user_dashboard",
      "issued_at": "2025-06-18T14:31:00Z"
    }
  ],
  "config_updates": {
    "moisture_threshold": 25.0,
    "telemetry_interval_sec": 10
  }
}
```

**Why HTTP polling and not WebSockets/MQTT?**
- Simpler on ESP32 — `HTTPClient` is built-in, no extra library.
- Firewalled networks friendly — outbound HTTP is almost always allowed.
- Scales to MQTT later — the *payload schema* is the same, only the transport changes.

**Tracking issue**: See `TODO.md` → "Edge gateway: bidirectional command channel (Phase 2)".

---

## Document References

- `docs/hardware.md` — BOM and wiring (pin assignments must match `device_id` in payload).
- `docs/GETTING_STARTED.md` — Calibration values feed directly into `raw_adc` fields.
- `AGENTS.md` — Coding conventions and secrets policy.
- `TODO.md` — Active tasks including Phase 2 command transport.

---

## Changelog

| Date | Change |
|------|--------|
| 2025-06-18 | Initial architecture doc: HTTP telemetry, SQLite schema, tiered authority, TimescaleDB future note, Phase 2 command stub. |
