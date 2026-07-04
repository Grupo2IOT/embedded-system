#include "WaterPump.h"

WaterPump::WaterPump(uint8_t actuatorPin) : BaseActuator(actuatorPin) {}

void WaterPump::begin() {
    pinMode(pin, OUTPUT);
    // Relay module is ACTIVE-LOW: HIGH = relay de-energized = pump OFF
    digitalWrite(pin, HIGH);
}

void WaterPump::execute(Command cmd) {
    if (cmd == Command::TURN_ON_WATER) {
        // ACTIVE-LOW: LOW energizes the relay coil → pump ON
        digitalWrite(pin, LOW);
    }
    else if (cmd == Command::TURN_OFF_WATER || cmd == Command::NONE) {
        // HIGH de-energizes the relay coil → pump OFF
        digitalWrite(pin, HIGH);
    }
    // Ignora los comandos que no le corresponden a este actuador
}