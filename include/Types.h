#ifndef TYPES_H
#define TYPES_H

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

#endif // TYPES_H