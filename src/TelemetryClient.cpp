#include "TelemetryClient.h"
#include "secrets.h"
#include <time.h>

TelemetryClient::TelemetryClient()
    : _tickCount(0),
      _lastWiFiAttempt(0),
      _txFailures(0) {}

void TelemetryClient::begin() {
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
}

void TelemetryClient::send(const CropState& state, const AgronomicDiagnosis& diagnosis) {
    _tickCount++;

    // Always print to Serial for local debugging
    _printSerial(state, diagnosis);

    // Attempt HTTP POST if WiFi is available
    if (_ensureWiFi()) {
        String payload = _buildJson(state, diagnosis);
        _sendHttp(payload);
    } else {
        Serial.println("[TELEMETRY] WiFi unavailable — packet dropped (Serial only).");
    }
}

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

    if (httpCode == 204 || httpCode == 200) {
        Serial.println("[TELEMETRY] HTTP POST OK (" + String(httpCode) + ")");
    } else if (httpCode > 0) {
        Serial.println("[TELEMETRY] HTTP POST returned " + String(httpCode));
        _txFailures++;
    } else {
        Serial.println("[TELEMETRY] HTTP POST failed: " + http.errorToString(httpCode));
        _txFailures++;
    }

    http.end();
}

String TelemetryClient::_buildJson(const CropState& state, const AgronomicDiagnosis& diagnosis) {
    StaticJsonDocument<1024> doc;

    // Meta
    JsonObject meta = doc.createNestedObject("meta");
    meta["device_id"] = DEVICE_ID;
    meta["firmware_version"] = "1.1.0";
    meta["tick_count"] = _tickCount;
    String ts = _isoTimestamp();
    if (ts == "null") {
        meta["timestamp_utc"] = nullptr;
    } else {
        meta["timestamp_utc"] = ts;
    }
    meta["wifi_rssi_dbm"] = WiFi.RSSI();

    // Sensors
    JsonObject sensors = doc.createNestedObject("sensors");

    JsonObject soilMoisture = sensors.createNestedObject("soil_moisture");
    soilMoisture["value"] = state.soilMoisture.isValid ? state.soilMoisture.percentage : nullptr;
    soilMoisture["unit"] = "%";
    soilMoisture["raw_adc"] = state.soilMoisture.rawValue;
    soilMoisture["is_valid"] = state.soilMoisture.isValid;

    JsonObject soilFertility = sensors.createNestedObject("soil_fertility");
    soilFertility["value"] = state.soilFertility.isValid ? state.soilFertility.conductivity : nullptr;
    soilFertility["unit"] = "mS/cm";
    soilFertility["raw_adc"] = state.soilFertility.rawValue;
    soilFertility["is_valid"] = state.soilFertility.isValid;

    JsonObject soilTemp = sensors.createNestedObject("soil_temperature");
    soilTemp["value"] = state.soilTemperature.isValid ? state.soilTemperature.celsius : nullptr;
    soilTemp["unit"] = "C";
    soilTemp["is_valid"] = state.soilTemperature.isValid;

    JsonObject air = sensors.createNestedObject("air");
    air["temperature"] = state.environment.isValid ? state.environment.temperature : nullptr;
    air["humidity"] = state.environment.isValid ? state.environment.humidity : nullptr;
    air["is_valid"] = state.environment.isValid;

    JsonObject waterLevel = sensors.createNestedObject("water_level");
    waterLevel["status"] = state.waterLevel.status == WaterLevelStatus::EMPTY ? "EMPTY" : "SUFFICIENT";
    waterLevel["is_valid"] = state.waterLevel.isValid;

    // Diagnosis
    JsonObject diag = doc.createNestedObject("diagnosis");
    diag["needs_irrigation"] = diagnosis.requiresIrrigation;
    diag["needs_fertilization"] = diagnosis.requiresFertilization;
    if (diagnosis.alertMessage != nullptr) {
        diag["alert_message"] = diagnosis.alertMessage;
    } else {
        diag["alert_message"] = nullptr;
    }

    // Actuators (inferred from diagnosis for Phase 1)
    JsonObject actuators = doc.createNestedObject("actuators");
    actuators["water_pump"] = diagnosis.requiresIrrigation ? "ON" : "OFF";
    actuators["fertilizer_pump"] = diagnosis.requiresFertilization ? "ON" : "OFF";

    // System health
    JsonObject health = doc.createNestedObject("system_health");
    int failures = 0;
    JsonArray failedSensors = health.createNestedArray("failed_sensors");
    if (!state.soilMoisture.isValid)    { failedSensors.add("soil_moisture");    failures++; }
    if (!state.soilFertility.isValid)   { failedSensors.add("soil_fertility");   failures++; }
    if (!state.soilTemperature.isValid) { failedSensors.add("soil_temperature"); failures++; }
    if (!state.environment.isValid)     { failedSensors.add("air");              failures++; }
    if (!state.waterLevel.isValid)      { failedSensors.add("water_level");      failures++; }

    health["overall"] = (failures == 0) ? "HEALTHY" : ((failures >= 3) ? "CRITICAL" : "DEGRADED");
    health.createNestedArray("pending_commands"); // Phase 2

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
