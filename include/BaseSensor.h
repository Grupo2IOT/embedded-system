#ifndef BASESENSOR_H
#define BASESENSOR_H

#include <Arduino.h> // Necesario para usar tipos nativos como uint8_t

class BaseSensor {
protected:
    uint8_t pin; // Pin GPIO asignado al sensor en el ESP32

public:
    // Constructor que inicializa el pin físico
    BaseSensor(uint8_t sensorPin) : pin(sensorPin) {}

    // Destructor virtual (Buena práctica en C++ para evitar memory leaks al usar herencia)
    virtual ~BaseSensor() {}

    // Método de inicialización (ej: configurar pinMode)
    virtual void begin() = 0; // = 0 significa que es una función virtual pura (abstracta)
};

#endif // BASESENSOR_H