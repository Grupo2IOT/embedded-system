#include "DHT22Sensor.h"
#include <cmath> // Necesario para usar std::isnan

DHT22Sensor::DHT22Sensor(uint8_t sensorPin) 
    : BaseSensor(sensorPin), dhtInstance(sensorPin, DHT22) {}

void DHT22Sensor::begin() {
    dhtInstance.begin();
    yield(); // Feed watchdog during DHT library init
}

EnvironmentReading DHT22Sensor::read() {
    EnvironmentReading reading;

    // 1. Consumir las lecturas digitales desde la librería de Adafruit
    float h = dhtInstance.readHumidity();
    float t = dhtInstance.readTemperature(); // Por defecto lee en Celsius

    // 2. Validación de Hardware: Si el cable de datos del DHT22 se desconecta,
    // la librería no devuelve un número, devuelve un estado eléctrico flotante "NaN"
    if (std::isnan(h) || std::isnan(t) || h < 0.0f || h > 100.0f || t < -40.0f || t > 80.0f) {
        reading.temperature = 0.0f;
        reading.humidity = 0.0f;
        reading.isValid = false; // Flag para alertar al pipeline de telemetría
        return reading;
    }

    // 3. Mapeo directo al DTO inmutable si todo está correcto
    reading.temperature = t;
    reading.humidity = h;
    reading.isValid = true;

    return reading;
}