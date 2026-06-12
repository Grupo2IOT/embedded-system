#ifndef TELEMETRYCLIENT_H
#define TELEMETRYCLIENT_H

#include "Readings.h"

class TelemetryClient {
public:
    TelemetryClient();
    void begin();
    
    // Recibe el estado actual y el diagnóstico para empaquetarlos
    void send(const CropState& state, const AgronomicDiagnosis& diagnosis);
};

#endif // TELEMETRYCLIENT_H