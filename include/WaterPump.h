#ifndef WATERPUMP_H
#define WATERPUMP_H

#include "BaseActuator.h"

class WaterPump : public BaseActuator {
public:
    WaterPump(uint8_t actuatorPin);
    void begin() override;
    void execute(Command cmd) override;
};

#endif // WATERPUMP_H