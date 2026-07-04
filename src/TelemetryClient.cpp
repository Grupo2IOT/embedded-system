#include "TelemetryClient.h"
#include "secrets.h"
#include <Arduino.h>
#include <time.h>

TelemetryClient::TelemetryClient()
    : _tickCount(0),
      _lastWiFiAttempt(0),
      _txFailures(0),
      _pendingCount(0) {
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; ++i) {
        _pendingCommands[i].valid = false;
    }
}

void TelemetryClient::begin() {
#if ENABLE_HTTP_TELEMETRY
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.print("[WIFI] Connecting to ");
    Serial.print(WIFI_SSID);

    unsigned long startAttempt = millis();
    const unsigned long CONNECTION_TIMEOUT_MS = 10000;

    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < CONNECTION_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(" OK");
        Serial.print("[WIFI] IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println(" TIMEOUT");
        Serial.println("[WIFI] Will retry in background. Local control loop continues.");
    }
#else
    Serial.println("[TELEMETRY] HTTP telemetry disabled. Serial output only.");
#endif
}

void TelemetryClient::send(const CropState& state, const AgronomicDiagnosis& diagnosis,
                           const CommandResult* cmdResults, uint8_t cmdResultCount) {
    _tickCount++;

    // Always print to Serial for local debugging
    _printSerial(state, diagnosis);

#if ENABLE_HTTP_TELEMETRY
    // Attempt HTTP POST if WiFi is available
    if (_ensureWiFi()) {
        String payload = _buildJson(state, diagnosis, cmdResults, cmdResultCount);
        _sendHttp(payload);
    } else {
        Serial.println("[TELEMETRY] WiFi unavailable — packet dropped (Serial only).");
    }
#endif
}

bool TelemetryClient::hasPendingCommands() const {
    return _pendingCount > 0;
}

uint8_t TelemetryClient::pendingCommandCount() const {
    return _pendingCount;
}

const RemoteCommand* TelemetryClient::pendingCommands() const {
    return _pendingCommands;
}

void TelemetryClient::clearPendingCommands() {
    _pendingCount = 0;
    for (uint8_t i = 0; i < MAX_PENDING_COMMANDS; ++i) {
        _pendingCommands[i].valid = false;
    }
}

#if ENABLE_HTTP_TELEMETRY
bool TelemetryClient::_ensureWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    unsigned long now = millis();
    if (now - _lastWiFiAttempt >= WIFI_RETRY_INTERVAL_MS) {
        _lastWiFiAttempt = now;
        Serial.println("[WIFI] Reconnecting...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    }

    return false;
}

void TelemetryClient::_sendHttp(const String& jsonPayload) {
    HTTPClient http;
    http.begin(EDGE_GATEWAY_URL);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-API-Key", API_KEY);

    int httpCode = http.POST(jsonPayload);

    if (httpCode == 200) {
        String response = http.getString();
        _parseResponse(response);
        Serial.println("[TELEMETRY] HTTP POST OK (200) — commands received");
    } else if (httpCode == 204) {
        Serial.println("[TELEMETRY] HTTP POST OK (204)");
    } else if (httpCode > 0) {
        Serial.println("[TELEMETRY] HTTP POST returned " + String(httpCode));
        _txFailures++;
    } else {
        Serial.println("[TELEMETRY] HTTP POST failed: " + http.errorToString(httpCode));
        _txFailures++;
    }

    http.end();
}

void TelemetryClient::_parseResponse(const String& responseBody) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, responseBody);
    if (err) {
        Serial.print("[TELEMETRY] Failed to parse response JSON: ");
        Serial.println(err.c_str());
        return;
    }

    JsonArray commands = doc["commands"];
    if (commands.isNull()) {
        return; // No commands field
    }

    _pendingCount = 0;
    for (JsonObject cmd : commands) {
        if (_pendingCount >= MAX_PENDING_COMMANDS) break;

        const char* target = cmd["target"];
        const char* state = cmd["state"];
        uint16_t duration = cmd["duration_sec"] | 10; // Default 10s if not specified

        if (target && state) {
            RemoteCommand& rc = _pendingCommands[_pendingCount++];
            strlcpy(rc.target, target, sizeof(rc.target));
            strlcpy(rc.state, state, sizeof(rc.state));
            rc.durationSec = duration;
            rc.valid = true;

            Serial.print("[COMMAND] Received: ");
            Serial.print(rc.target);
            Serial.print(" -> ");
            Serial.print(rc.state);
            Serial.print(" for ");
            Serial.print(rc.durationSec);
            Serial.println("s");
        }
    }
}

String TelemetryClient::_buildJson(const CropState& state, const AgronomicDiagnosis& diagnosis,
                                   const CommandResult* cmdResults, uint8_t cmdResultCount) {
    JsonDocument doc;

    // Meta
    JsonObject meta = doc["meta"].to<JsonObject>();
    meta["device_id"] = DEVICE_ID;
    meta["firmware_version"] = "1.2.0";
    meta["tick_count"] = _tickCount;
    String ts = _isoTimestamp();
    if (ts == "null") {
        meta["timestamp_utc"] = nullptr;
    } else {
        meta["timestamp_utc"] = ts;
    }
    meta["wifi_rssi_dbm"] = WiFi.RSSI();

    // Sensors
    JsonObject sensors = doc["sensors"].to<JsonObject>();

    JsonObject soilMoisture = sensors["soil_moisture"].to<JsonObject>();
    if (state.soilMoisture.isValid) {
        soilMoisture["value"] = state.soilMoisture.percentage;
    } else {
        soilMoisture["value"] = nullptr;
    }
    soilMoisture["unit"] = "%";
    soilMoisture["raw_adc"] = state.soilMoisture.rawValue;
    soilMoisture["is_valid"] = state.soilMoisture.isValid;

    JsonObject soilFertility = sensors["soil_fertility"].to<JsonObject>();
    if (state.soilFertility.isValid) {
        soilFertility["value"] = state.soilFertility.conductivity;
    } else {
        soilFertility["value"] = nullptr;
    }
    soilFertility["unit"] = "mS/cm";
    soilFertility["raw_adc"] = state.soilFertility.rawValue;
    soilFertility["is_valid"] = state.soilFertility.isValid;

    JsonObject soilTemp = sensors["soil_temperature"].to<JsonObject>();
    if (state.soilTemperature.isValid) {
        soilTemp["value"] = state.soilTemperature.celsius;
    } else {
        soilTemp["value"] = nullptr;
    }
    soilTemp["unit"] = "C";
    soilTemp["is_valid"] = state.soilTemperature.isValid;

    JsonObject air = sensors["air"].to<JsonObject>();
    if (state.environment.isValid) {
        air["temperature"] = state.environment.temperature;
        air["humidity"] = state.environment.humidity;
    } else {
        air["temperature"] = nullptr;
        air["humidity"] = nullptr;
    }
    air["is_valid"] = state.environment.isValid;

    JsonObject waterLevel = sensors["water_level"].to<JsonObject>();
    waterLevel["status"] = state.waterLevel.status == WaterLevelStatus::EMPTY ? "EMPTY" : "SUFFICIENT";
    waterLevel["is_valid"] = state.waterLevel.isValid;

    // Diagnosis
    JsonObject diag = doc["diagnosis"].to<JsonObject>();
    diag["needs_irrigation"] = diagnosis.requiresIrrigation;
    diag["needs_fertilization"] = diagnosis.requiresFertilization;
    if (diagnosis.alertMessage != nullptr) {
        diag["alert_message"] = diagnosis.alertMessage;
    } else {
        diag["alert_message"] = nullptr;
    }

    // Actuators (actual state, may differ from diagnosis if override is active)
    JsonObject actuators = doc["actuators"].to<JsonObject>();
    actuators["water_pump"] = diagnosis.requiresIrrigation ? "ON" : "OFF";
    actuators["fertilizer_pump"] = diagnosis.requiresFertilization ? "ON" : "OFF";

    // System health
    JsonObject health = doc["system_health"].to<JsonObject>();
    int failures = 0;
    JsonArray failedSensors = health["failed_sensors"].to<JsonArray>();
    if (!state.soilMoisture.isValid)    { failedSensors.add("soil_moisture");    failures++; }
    if (!state.soilFertility.isValid)   { failedSensors.add("soil_fertility");   failures++; }
    if (!state.soilTemperature.isValid) { failedSensors.add("soil_temperature"); failures++; }
    if (!state.environment.isValid)     { failedSensors.add("air");              failures++; }
    if (!state.waterLevel.isValid)      { failedSensors.add("water_level");      failures++; }

    health["overall"] = (failures == 0) ? "HEALTHY" : ((failures >= 3) ? "CRITICAL" : "DEGRADED");

    // Phase 2: command results
    JsonArray pendingCmds = health["pending_commands"].to<JsonArray>();
    for (uint8_t i = 0; i < cmdResultCount; ++i) {
        if (!cmdResults[i].valid) continue;
        JsonObject cr = pendingCmds.add<JsonObject>();
        cr["command"] = cmdResults[i].command;
        cr["executed"] = cmdResults[i].executed;
        cr["reason"] = cmdResults[i].reason;
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String TelemetryClient::_isoTimestamp() const {
    // Phase 1: No NTP sync yet. Returns epoch or a placeholder.
    // Phase 2: Add NTP client and format actual UTC time here.
    time_t now = time(nullptr);
    if (now < 1000000000) {
        return "null"; // Not yet synced — edge should use received_at
    }

    struct tm timeinfo;
    gmtime_r(&now, &timeinfo);
    char buf[25];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return String(buf);
}
#endif

void TelemetryClient::_printSerial(const CropState& state, const AgronomicDiagnosis& diagnosis) {
    Serial.println("\n--- [TELEMETRY PACKET] ---");

    if (state.soilMoisture.isValid) {
        Serial.print("  > Soil Moisture: "); Serial.print(state.soilMoisture.percentage); Serial.print("% (raw: "); Serial.print(state.soilMoisture.rawValue); Serial.println(")");
    } else {
        Serial.print("  > Soil Moisture: [HARDWARE_ERROR] (raw: "); Serial.print(state.soilMoisture.rawValue); Serial.println(")");
    }

    if (state.soilFertility.isValid) {
        Serial.print("  > Soil Fertility (EC): "); Serial.print(state.soilFertility.conductivity); Serial.print(" mS/cm (raw: "); Serial.print(state.soilFertility.rawValue); Serial.println(")");
    } else {
        Serial.print("  > Soil Fertility: [HARDWARE_ERROR] (raw: "); Serial.print(state.soilFertility.rawValue); Serial.println(")");
    }

    if (state.soilTemperature.isValid) {
        Serial.print("  > Soil Temp: "); Serial.print(state.soilTemperature.celsius); Serial.println(" C");
    } else {
        Serial.println("  > Soil Temp: [HARDWARE_ERROR]");
    }

    if (state.environment.isValid) {
        Serial.print("  > Air Temp: "); Serial.print(state.environment.temperature); Serial.print(" C | ");
        Serial.print("Air Humidity: "); Serial.print(state.environment.humidity); Serial.println("%");
    } else {
        Serial.println("  > Environment (DHT22): [HARDWARE_ERROR]");
    }

    Serial.print("  > Irrigation Status: "); Serial.println(diagnosis.requiresIrrigation ? "ACTIVE" : "OFF");
    Serial.print("  > Fertilization Status: "); Serial.println(diagnosis.requiresFertilization ? "ACTIVE" : "OFF");
    Serial.print("  > System Log: "); Serial.println(diagnosis.alertMessage ? diagnosis.alertMessage : "None");
    Serial.println("---------------------------\n");
}