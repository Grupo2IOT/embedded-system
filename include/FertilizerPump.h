#ifndef FERTILIZERPUMP_H
#define FERTILIZERPUMP_H

#include "BaseActuator.h"

class FertilizerPump : public BaseActuator {
public:
    FertilizerPump(uint8_t actuatorPin);
    void begin() override;
    void execute(Command cmd) override;
};

#endif // FERTILIZERPUMP_H