#ifndef WATERLEVELSENSOR_H
#define WATERLEVELSENSOR_H

#include "BaseSensor.h"
#include "Readings.h"

class WaterLevelSensor : public BaseSensor {
public:
    // Constructor: Pasa el pin a la clase BaseSensor
    WaterLevelSensor(uint8_t sensorPin);

    // Inicialización del pin digital
    void begin() override;

    // Método que evalúa el estado del tanque y devuelve el DTO
    WaterLevelReading read();
};

#endif // WATERLEVELSENSOR_H