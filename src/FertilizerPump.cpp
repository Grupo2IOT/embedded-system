#include "FertilizerPump.h"

FertilizerPump::FertilizerPump(uint8_t actuatorPin) : BaseActuator(actuatorPin) {}

void FertilizerPump::begin() {
    pinMode(pin, OUTPUT);
    // Relay module is ACTIVE-LOW: HIGH = relay de-energized = pump OFF
    digitalWrite(pin, HIGH);
}

void FertilizerPump::execute(Command cmd) {
    if (cmd == Command::TURN_ON_FERTILIZER) {
        // ACTIVE-LOW: LOW energizes the relay coil → pump ON
        digitalWrite(pin, LOW);
    }
    else if (cmd == Command::TURN_OFF_FERTILIZER || cmd == Command::NONE) {
        // HIGH de-energizes the relay coil → pump OFF
        digitalWrite(pin, HIGH);
    }
}