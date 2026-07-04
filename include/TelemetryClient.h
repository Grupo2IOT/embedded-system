#ifndef TELEMETRYCLIENT_H
#define TELEMETRYCLIENT_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Readings.h"
#include "Types.h"

class TelemetryClient {
public:
    TelemetryClient();
    void begin();
    void send(const CropState& state, const AgronomicDiagnosis& diagnosis,
              const CommandResult* cmdResults = nullptr, uint8_t cmdResultCount = 0);

    // Phase 2: retrieve commands piggybacked in the telemetry response
    bool hasPendingCommands() const;
    uint8_t pendingCommandCount() const;
    const RemoteCommand* pendingCommands() const;
    void clearPendingCommands();

private:
    unsigned long _tickCount;
    unsigned long _lastWiFiAttempt;
    unsigned long _txFailures;
    const unsigned long WIFI_RETRY_INTERVAL_MS = 30000; // 30 seconds

    RemoteCommand _pendingCommands[MAX_PENDING_COMMANDS];
    uint8_t _pendingCount;

    bool _ensureWiFi();
    void _sendHttp(const String& jsonPayload);
    void _parseResponse(const String& responseBody);
    String _buildJson(const CropState& state, const AgronomicDiagnosis& diagnosis, const CommandResult* cmdResults, uint8_t cmdResultCount);
    String _isoTimestamp() const;
    void _printSerial(const CropState& state, const AgronomicDiagnosis& diagnosis);
};

#endif // TELEMETRYCLIENT_H
