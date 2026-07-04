#ifndef IRRIGATIONCONTROLLER_H
#define IRRIGATIONCONTROLLER_H

#include "SoilMoistureSensor.h"
#include "SoilFertilitySensor.h"
#include "SoilTemperatureSensor.h"
#include "DHT22Sensor.h"
#include "WaterLevelSensor.h"
#include "AgronomicEvaluator.h"
#include "WaterPump.h"
#include "FertilizerPump.h"
#include "TelemetryClient.h"
#include "Types.h"

class IrrigationController {
private:
    // Instancias de Componentes mediante Composición
    SoilMoistureSensor     _moistureSensor;
    SoilFertilitySensor    _fertilitySensor;
    SoilTemperatureSensor  _tempSensor;
    DHT22Sensor            _dhtSensor;
    WaterLevelSensor       _waterLevelSensor;

    AgronomicEvaluator     _evaluador;

    WaterPump              _waterPump;
    FertilizerPump         _fertilizerPump;

    TelemetryClient        _telemetry;

    // Control de Tiempos Asíncronos
    unsigned long _lastTick;
    unsigned long _lastTelemetry;
    const unsigned long TICK_INTERVAL = 5000; // Ejecutar ciclo cada 5 segundos (5000ms)
    const unsigned long TELEMETRY_INTERVAL = 5000; // Reportar telemetría cada 5 segundos

    // Phase 2: Remote override state tracking (fixed-duration commands)
    struct OverrideState {
        bool active = false;
        unsigned long endTime = 0;
    };
    OverrideState _waterOverride;
    OverrideState _fertilizerOverride;

    static constexpr uint8_t MAX_CMD_RESULTS = 4;
    CommandResult _cmdResults[MAX_CMD_RESULTS];
    uint8_t _cmdResultCount = 0;

    void _checkOverrideTimeouts(unsigned long now);
    void _processRemoteCommands(const CropState& state, unsigned long now);
    void _reportCommandResult(const char* command, bool executed, const char* reason);
    void _driveActuators(AgronomicDiagnosis& diagnosis, unsigned long now);

public:
    // El constructor recibe los mapeos de pines asignados de todo el sistema
    IrrigationController(
        uint8_t pinMoisture, uint8_t pinFertility, uint8_t pinTemp, uint8_t pinDHT, uint8_t pinWaterLevel,
        uint8_t pinWaterPump, uint8_t pinFertilizerPump
    );

    void begin();

    // Método cíclico principal (se invoca en el loop sin bloquear)
    void tick();
};

#endif // IRRIGATIONCONTROLLER_H