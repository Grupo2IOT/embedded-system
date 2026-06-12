#include "TelemetryClient.h"
#include <Arduino.h>

TelemetryClient::TelemetryClient() {}

void TelemetryClient::begin() {
    // Aquí iría la inicialización de WiFi o cliente MQTT en el futuro
}

void TelemetryClient::send(const CropState& state, const AgronomicDiagnosis& diagnosis) {
    Serial.println("\n--- [TELEMETRY PACKET] ---");
    
    // Volcado de Sensores usando tus campos en inglés
    if (state.soilMoisture.isValid) {
        Serial.print("  > Soil Moisture: "); Serial.print(state.soilMoisture.percentage); Serial.println("%");
    } else {
        Serial.println("  > Soil Moisture: [HARDWARE_ERROR]");
    }

    if (state.soilFertility.isValid) {
        Serial.print("  > Soil Fertility (EC): "); Serial.print(state.soilFertility.conductivity); Serial.println(" mS/cm");
    } else {
        Serial.println("  > Soil Fertility: [HARDWARE_ERROR]");
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

    // Diagnóstico y alertas finales de tu capa de negocio
    Serial.print("  > Irrigation Status: "); Serial.println(diagnosis.requiresIrrigation ? "ACTIVE" : "OFF");
    Serial.print("  > Fertilization Status: "); Serial.println(diagnosis.requiresFertilization ? "ACTIVE" : "OFF");
    Serial.print("  > System Log: "); Serial.println(diagnosis.alertMessage);
    Serial.println("---------------------------\n");
}