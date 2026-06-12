#ifndef BASEACTUATOR_H
#define BASEACTUATOR_H

#include <Arduino.h>
#include "Types.h"

class BaseActuator {
protected:
    uint8_t pin; // Pin GPIO asignado al actuador en el ESP32

public:
    // Constructor que inicializa el pin físico
    BaseActuator(uint8_t actuatorPin) : pin(actuatorPin) {}

    // Destructor virtual para evitar memory leaks
    virtual ~BaseActuator() {}

    // Método de inicialización obligatorio (ej: configurar pinMode como OUTPUT)
    virtual void begin() = 0;

    // Método abstracto para ejecutar un comando específico de negocio
    virtual void execute(Command cmd) = 0;
};

#endif // BASEACTUATOR_H