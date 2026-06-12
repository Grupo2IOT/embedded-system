#include "AgronomicEvaluator.h"

AgronomicEvaluator::AgronomicEvaluator() {}

AgronomicDiagnosis AgronomicEvaluator::evaluate(const CropState& state) {
    AgronomicDiagnosis diagnosis;
    
    // Inicializamos por defecto los estados del DTO de diagnóstico
    diagnosis.requiresIrrigation = false;
    diagnosis.requiresFertilization = false;
    diagnosis.alertMessage = "SYSTEM_OK";

    // 1. REGLA DE SEGURIDAD CRÍTICA: Verificación del tanque de agua
    if (state.waterLevel.isValid && state.waterLevel.status == WaterLevelStatus::EMPTY) {
        diagnosis.requiresIrrigation = false;
        diagnosis.requiresFertilization = false;
        diagnosis.alertMessage = "CRITICAL_ALERT: Water tank is empty! Pumps disabled.";
        return diagnosis; // Cortocircuitamos el flujo por seguridad para no fundir las bombas
    }

    // 2. REGLA DE RIEGO (Humedad del suelo)
    if (state.soilMoisture.isValid) {
        if (state.soilMoisture.percentage < MOISTURE_THRESHOLD) {
            diagnosis.requiresIrrigation = true;
            diagnosis.alertMessage = "ALERT: Low soil moisture. Requesting irrigation.";
        }
    } else {
        // Si el hardware falla, no arriesgamos el cultivo pero alertamos al pipeline
        diagnosis.alertMessage = "HARDWARE_ERROR: Soil moisture sensor failure.";
    }

    // 3. REGLA DE FERTILIZACIÓN (Conductividad eléctrica / Nutrientes)
    if (state.soilFertility.isValid) {
        if (state.soilFertility.conductivity < FERTILITY_THRESHOLD) {
            diagnosis.requiresFertilization = true;
            // Si ya había una alerta de humedad, concatenamos el mensaje conceptualmente
            if (diagnosis.requiresIrrigation) {
                diagnosis.alertMessage = "ALERT: Low moisture AND low nutrients detected.";
            } else {
                diagnosis.alertMessage = "ALERT: Low nutrient levels detected.";
            }
        }
    } else {
        if (state.soilMoisture.isValid && state.soilMoisture.percentage < MOISTURE_THRESHOLD) {
            diagnosis.alertMessage = "ALERT: Low moisture. Fert. sensor hardware failure.";
        } else {
            diagnosis.alertMessage = "HARDWARE_ERROR: Soil fertility sensor failure.";
        }
    }

    return diagnosis;
}