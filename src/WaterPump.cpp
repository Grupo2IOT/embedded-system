#include "WaterPump.h"

WaterPump::WaterPump(uint8_t actuatorPin) : BaseActuator(actuatorPin) {}

void WaterPump::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Aseguramos que la bomba inicie apagada por seguridad
}

void WaterPump::execute(Command cmd) {
    if (cmd == Command::TURN_ON_WATER) {
        digitalWrite(pin, HIGH); // Activa el relé de la bomba de agua
    } 
    else if (cmd == Command::TURN_OFF_WATER || cmd == Command::NONE) {
        digitalWrite(pin, LOW);  // Apaga la bomba de agua
    }
    // Ignora los comandos que no le corresponden a este actuador
}