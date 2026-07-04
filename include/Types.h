#ifndef TYPES_H
#define TYPES_H

#include <Arduino.h>

// Comandos de acción que el controlador puede ejecutar en los actuadores
enum class Command {
    NONE,
    TURN_ON_WATER,
    TURN_OFF_WATER,
    TURN_ON_FERTILIZER,
    TURN_OFF_FERTILIZER
};

// Estados posibles del nivel de los tanques
enum class WaterLevelStatus {
    EMPTY,
    SUFFICIENT
};

// ---------------------------------------------------------------------------
// Phase 2: Remote command structures
// ---------------------------------------------------------------------------

// Maximum number of remote commands that can be queued per telemetry cycle
constexpr uint8_t MAX_PENDING_COMMANDS = 4;

// Parsed remote command received from the edge gateway
struct RemoteCommand {
    char target[20];      // e.g. "water_pump", "fertilizer_pump"
    char state[8];        // e.g. "ON", "OFF"
    uint16_t durationSec; // 0 = indefinite (evaluator-controlled), >0 = fixed override
    bool valid = false;   // true if this slot contains an active command
};

// Result of a command execution for reporting back to the edge
struct CommandResult {
    char command[64];
    bool executed;
    char reason[64];
    bool valid = false;
};

#endif // TYPES_H