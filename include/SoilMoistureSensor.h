#ifndef SOILMOISTURESENSOR_H
#define SOILMOISTURESENSOR_H

#include "BaseSensor.h"
#include "Readings.h"

class SoilMoistureSensor : public BaseSensor {
private:
    // Valores de calibración típicos para el ADC de 12 bits del ESP32
    // Nota: Estos valores se ajustan con pruebas reales metiendo el sensor en agua y aire
    const uint16_t AIR_VALUE = 3200;  // Valor crudo en seco (0%)
    const uint16_t WATER_VALUE = 1500; // Valor crudo en agua (100%)

public:
    // Constructor: Pasa el pin a la clase base usando la lista de inicialización
    SoilMoistureSensor(uint8_t sensorPin);

    // Implementación obligatoria del método de la interfaz/clase abstracta
    void begin() override;

    // Método propio para obtener el DTO de lectura estructurado
    SoilMoistureReading read();
};

#endif // SOILMOISTURESENSOR_H