#include "WaterLevelSensor.h"

// Enlazamos el constructor y pasamos el pin al padre con la sintaxis correcta
WaterLevelSensor::WaterLevelSensor(uint8_t sensorPin) : BaseSensor(sensorPin) {}

void WaterLevelSensor::begin() {
    // Configuramos el pin del ESP32 como entrada digital simple
    pinMode(pin, INPUT);
}

WaterLevelReading WaterLevelSensor::read() {
    WaterLevelReading reading;

    // 1. Leer el estado digital del pin (HIGH o LOW)
    int pinState = digitalRead(pin);

    // 2. Mapear el estado físico al Enum lógico de negocio
    // Asumiendo configuración Pull-Down: HIGH = Boya flotando (Suficiente agua)
    if (pinState == HIGH) {
        reading.status = WaterLevelStatus::SUFFICIENT;
    } else {
        reading.status = WaterLevelStatus::EMPTY;
    }

    // 3. Al ser un interruptor mecánico simple respaldado por una resistencia física,
    // a menos que el cable se rompa, la lectura siempre se considera válida para el pipeline.
    reading.isValid = true;

    return reading;
}