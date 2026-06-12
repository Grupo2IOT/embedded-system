#include "FertilizerPump.h"

FertilizerPump::FertilizerPump(uint8_t actuatorPin) : BaseActuator(actuatorPin) {}

void FertilizerPump::begin() {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW); // Aseguramos que la bomba inicie apagada
}

void FertilizerPump::execute(Command cmd) {
    if (cmd == Command::TURN_ON_FERTILIZER) {
        digitalWrite(pin, HIGH); // Activa el relé de fertilizante
    } 
    else if (cmd == Command::TURN_OFF_FERTILIZER || cmd == Command::NONE) {
        digitalWrite(pin, LOW);  // Apaga la bomba de fertilizante
    }
}