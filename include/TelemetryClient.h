#ifndef TELEMETRYCLIENT_H
#define TELEMETRYCLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Readings.h"

class TelemetryClient {
public:
    TelemetryClient();
    void begin();
    void send(const CropState& state, const AgronomicDiagnosis& diagnosis);

private:
    unsigned long _tickCount;
    unsigned long _lastWiFiAttempt;
    unsigned long _txFailures;
    const unsigned long WIFI_RETRY_INTERVAL_MS = 30000; // 30 seconds

    bool _ensureWiFi();
    void _sendHttp(const String& jsonPayload);
    String _buildJson(const CropState& state, const AgronomicDiagnosis& diagnosis);
    String _isoTimestamp() const;
    void _printSerial(const CropState& state, const AgronomicDiagnosis& diagnosis);
};

#endif // TELEMETRYCLIENT_H
