#include "IrrigationController.h"

IrrigationController::IrrigationController(
    uint8_t pinMoisture, uint8_t pinFertility, uint8_t pinTemp, uint8_t pinDHT, uint8_t pinWaterLevel,
    uint8_t pinWaterPump, uint8_t pinFertilizerPump
) : 
    _moistureSensor(pinMoisture),
    _fertilitySensor(pinFertility),
    _tempSensor(pinTemp),
    _dhtSensor(pinDHT),
    _waterLevelSensor(pinWaterLevel),
    _waterPump(pinWaterPump),
    _fertilizerPump(pinFertilizerPump),
    _lastTick(0),
    _lastTelemetry(0) {}

void IrrigationController::begin() {
    // Inicializar todo el hardware registrado
    // Se intercala yield() entre cada sensor para alimentar el watchdog
    _moistureSensor.begin();
    yield();
    _fertilitySensor.begin();
    yield();
    _tempSensor.begin();
    yield();
    _dhtSensor.begin();
    yield();
    _waterLevelSensor.begin();
    yield();
    
    _waterPump.begin();
    yield();
    _fertilizerPump.begin();
    yield();
    _telemetry.begin();
}

void IrrigationController::tick() {
    unsigned long currentMillis = millis();

    // Verificamos si ya transcurrió el intervalo (No Bloqueante)
    if (currentMillis - _lastTick >= TICK_INTERVAL) {
        _lastTick = currentMillis;

        // 1. CAPTACIÓN: Recolectar datos de hardware mapeados a los DTOs
        CropState currentState;
        currentState.soilMoisture    = _moistureSensor.read();
        currentState.soilFertility   = _fertilitySensor.read();
        currentState.soilTemperature = _tempSensor.read();
        currentState.environment     = _dhtSensor.read();
        currentState.waterLevel      = _waterLevelSensor.read();

        // 2. DOMINIO: Evaluar el estado según las reglas de negocio
        AgronomicDiagnosis diagnosis = _evaluador.evaluate(currentState);

        // 3. ACCIÓN: Transformar diagnóstico en comandos físicos para los actuadores
        if (diagnosis.requiresIrrigation) {
            _waterPump.execute(Command::TURN_ON_WATER);
        } else {
            _waterPump.execute(Command::TURN_OFF_WATER);
        }

        if (diagnosis.requiresFertilization) {
            _fertilizerPump.execute(Command::TURN_ON_FERTILIZER);
        } else {
            _fertilizerPump.execute(Command::TURN_OFF_FERTILIZER);
        }

        // 4. TELEMETRÍA: Reportar el estado consolidado (solo cada 30s para reducir ruido)
        if (currentMillis - _lastTelemetry >= TELEMETRY_INTERVAL) {
            _lastTelemetry = currentMillis;
            _telemetry.send(currentState, diagnosis);
        }
    }
}