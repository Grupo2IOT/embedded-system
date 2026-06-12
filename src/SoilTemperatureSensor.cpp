#include "SoilTemperatureSensor.h"

// Usamos la lista de inicialización para asociar el pin físico con la instancia de OneWire
// Luego, le pasamos esa instancia de OneWire a la librería de DallasTemperature
SoilTemperatureSensor::SoilTemperatureSensor(uint8_t sensorPin) 
    : BaseSensor(sensorPin), oneWireInstance(sensorPin), dallasSensor(&oneWireInstance) {}

void SoilTemperatureSensor::begin() {
    // Inicializa el bus OneWire y configura la resolución interna del chip (9 a 12 bits)
    dallasSensor.begin();
    
    // Opcional: Bajamos la resolución a 10 bits para que la lectura sea más rápida (tarda menos milisegundos)
    // El ESP32 no se quedará esperando colgado tanto tiempo en el ciclo
    dallasSensor.setWaitForConversion(true); 
}

SoilTemperatureReading SoilTemperatureSensor::read() {
    SoilTemperatureReading reading;

    // 1. Mandar una orden global al bus: "Todos los chips DS18B20, midan la temperatura ahorita"
    dallasSensor.requestTemperatures();

    // 2. Leer el valor en Celsius del primer dispositivo encontrado en ese pin (índice 0)
    float tempC = dallasSensor.getTempCByIndex(0);

    // 3. Validación de Hardware: Si la sonda se desconecta o la resistencia de pull-up se cae,
    // la librería de Dallas Temperature devuelve un valor constante de error: DEVICE_DISCONNECTED_C (-127.0)
    if (tempC == DEVICE_DISCONNECTED_C || tempC < -50.0f || tempC > 125.0f) {
        reading.celsius = 0.0f;
        reading.isValid = false; // Flag de error para aislar el pipeline en la telemetría
        return reading;
    }

    // 4. Carga de datos limpios en el DTO
    reading.celsius = tempC;
    reading.isValid = true;

    return reading;
}