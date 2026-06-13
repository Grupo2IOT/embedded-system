#include "SoilMoistureSensor.h"

// Llamamos explícitamente al constructor de la clase padre
SoilMoistureSensor::SoilMoistureSensor(uint8_t sensorPin) : BaseSensor(sensorPin) {}

void SoilMoistureSensor::begin() {
    // Configuramos el pin GPIO como entrada analógica
    pinMode(pin, INPUT);
}

SoilMoistureReading SoilMoistureSensor::read() {
    SoilMoistureReading reading;
    
    // 1. Leer el valor analógico crudo del ADC (0 - 4095)
    int rawValue = analogRead(pin);
    reading.rawValue = rawValue; // Guardar el valor crudo para calibración

    // 2. Validación de Hardware: Si el cable se desconecta, el ADC suele flotar a 0 o 4095
    if (rawValue < 500 || rawValue > 4050) {
        reading.percentage = 0.0f;
        reading.isValid = false; // Flag de error para el pipeline de telemetría
        return reading;
    }

    // 3. Mapeo matemático inverso (A menor voltaje/valor crudo, mayor humedad)
    float percentage = 100.0f * (float)(AIR_VALUE - rawValue) / (float)(AIR_VALUE - WATER_VALUE);

    // 4. Clampear el valor entre 0% y 100% para evitar desbordes matemáticos por ruido eléctrico
    if (percentage < 0.0f) percentage = 0.0f;
    if (percentage > 100.0f) percentage = 100.0f;

    reading.percentage = percentage;
    reading.isValid = true;

    return reading;
}