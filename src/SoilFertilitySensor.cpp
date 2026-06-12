#include "SoilFertilitySensor.h"

// Pasamos el pin hacia el constructor de la clase padre (como el super() de Java)
SoilFertilitySensor::SoilFertilitySensor(uint8_t sensorPin) : BaseSensor(sensorPin) {}

void SoilFertilitySensor::begin() {
    pinMode(pin, INPUT);
}

SoilFertilityReading SoilFertilitySensor::read() {
    SoilFertilityReading reading;

    // 1. Leer el valor analógico del pin del ESP32 (0 - 4095)
    int rawValue = analogRead(pin);

    // 2. Validación de Hardware contra falsos contactos o desconexión física
    if (rawValue < 200 || rawValue > 4080) {
        reading.conductivity = 0.0f;
        reading.isValid = false; // Flag de error para aislar el pipeline
        return reading;
    }

    // 3. Transformación matemática para estimar la presencia de nutrientes (EC)
    // Invertimos la escala: a menor valor de ADC (menor resistencia), mayor conductividad
    float estimatedConductivity = (float)(MAX_RESISTANCE - rawValue) / (float)(MAX_RESISTANCE - MIN_RESISTANCE);

    // Multiplicamos por un factor de escala indexado (ej. escala de 0.0 a 5.0 mS/cm simulados)
    // Esto facilitará que tu Edge API reciba un número limpio y estandarizado
    float calculatedEc = estimatedConductivity * 5.0f; 

    // 4. Clampear valores para mitigar ruidos eléctricos en el protoboard
    if (calculatedEc < 0.0f) calculatedEc = 0.0f;
    if (calculatedEc > 5.0f) calculatedEc = 5.0f;

    reading.conductivity = calculatedEc;
    reading.isValid = true;

    return reading;
}