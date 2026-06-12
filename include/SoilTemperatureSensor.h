#ifndef SOILTEMPERATURESENSOR_H
#define SOILTEMPERATURESENSOR_H

#include "BaseSensor.h"
#include "Readings.h"
#include <OneWire.h>           // Librería de bajo nivel para el bus
#include <DallasTemperature.h> // Librería de alto nivel para el chip

class SoilTemperatureSensor : public BaseSensor {
private:
    // Creamos las instancias necesarias para gestionar la comunicación digital
    OneWire oneWireInstance;
    DallasTemperature dallasSensor;

public:
    // Constructor
    SoilTemperatureSensor(uint8_t sensorPin);

    // Inicialización del bus OneWire
    void begin() override;

    // Método para capturar la temperatura en formato DTO
    SoilTemperatureReading read();
};

#endif // SOILTEMPERATURESENSOR_H