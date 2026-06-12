#ifndef DHT22SENSOR_H
#define DHT22SENSOR_H

#include "BaseSensor.h"
#include "Readings.h"
#include <DHT.h> // Librería oficial de Adafruit para sensores DHT

class DHT22Sensor : public BaseSensor {
private:
    DHT dhtInstance; // Instancia interna de la librería de Adafruit

public:
    // Constructor: Pasa el pin a la base e inicializa la instancia interna
    // El segundo parámetro le dice a la librería que físicamente es un DHT22 (y no un DHT11)
    DHT22Sensor(uint8_t sensorPin);

    // Inicialización del sensor (configuración del protocolo)
    void begin() override;

    // Método que procesa las variables climáticas y devuelve el DTO compuesto
    EnvironmentReading read();
};

#endif // DHT22SENSOR_H