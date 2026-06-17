#ifndef SOILFERTILITYSENSOR_H
#define SOILFERTILITYSENSOR_H

#include "BaseSensor.h"
#include "Readings.h"

class SoilFertilitySensor : public BaseSensor {
private:
    // Valores de calibración típicos para conductividad eléctrica (EC) cruda
    // Se calibran metiendo el sensor en agua destilada (baja fertilidad) 
    // versus agua con fertilizante/compost disuelto (alta fertilidad)
    const uint16_t MAX_RESISTANCE = 4095; // Suelo pobre en nutrientes (Voltaje alto) — calibrado
    const uint16_t MIN_RESISTANCE = 1500; // Suelo óptimo/saturado de nutrientes (Voltaje bajo) — calibrado con buffer

public:
    // Constructor que delega el pin a BaseSensor
    SoilFertilitySensor(uint8_t sensorPin);

    // Implementación del contrato de inicialización
    void begin() override;

    // Método para resolver la lectura y devolver el DTO específico
    SoilFertilityReading read();
};

#endif // SOILFERTILITYSENSOR_H