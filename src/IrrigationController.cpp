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
    _lastTick(0) {}

void IrrigationController::begin() {
    // Inicializar todo el hardware registrado
    _moistureSensor.begin();
    _fertilitySensor.begin();
    _tempSensor.begin();
    _dhtSensor.begin();
    _waterLevelSensor.begin();
    
    _waterPump.begin();
    _fertilitySensor.begin(); // Inicialización preventiva del pin analógico alterno
    _fertilizerPump.begin();
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

        // 4. TELEMETRÍA: Reportar el estado consolidado
        _telemetry.send(currentState, diagnosis);
    }
}