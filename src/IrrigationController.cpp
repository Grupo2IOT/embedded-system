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

        // 0. Clear command results from previous cycle (already sent in last telemetry)
        _cmdResultCount = 0;

        // 1. Check override timeouts (turn off expired overrides)
        _checkOverrideTimeouts(currentMillis);

        // 2. CAPTACIÓN: Recolectar datos de hardware mapeados a los DTOs
        CropState currentState;
        currentState.soilMoisture    = _moistureSensor.read();
        currentState.soilFertility   = _fertilitySensor.read();
        currentState.soilTemperature = _tempSensor.read();
        currentState.environment     = _dhtSensor.read();
        currentState.waterLevel      = _waterLevelSensor.read();

        // 3. PHASE 2: Process remote commands piggybacked from previous telemetry response
        //    Must happen after sensor read so Tier 1 safety can check water level.
        _processRemoteCommands(currentState, currentMillis);

        // 4. DOMINIO: Evaluar el estado según las reglas de negocio
        AgronomicDiagnosis diagnosis = _evaluador.evaluate(currentState);

        // 5. ACCIÓN: Drive actuators (override takes precedence over evaluator)
        _driveActuators(diagnosis, currentMillis);

        // 6. TELEMETRÍA: Reportar el estado consolidado (solo cada 30s para reducir ruido)
        if (currentMillis - _lastTelemetry >= TELEMETRY_INTERVAL) {
            _lastTelemetry = currentMillis;
            _telemetry.send(currentState, diagnosis, _cmdResults, _cmdResultCount);
        }
    }
}

void IrrigationController::_checkOverrideTimeouts(unsigned long now) {
    if (_waterOverride.active && now >= _waterOverride.endTime) {
        _waterOverride.active = false;
        Serial.println("[OVERRIDE] Water pump override expired — returning to evaluator control.");
    }
    if (_fertilizerOverride.active && now >= _fertilizerOverride.endTime) {
        _fertilizerOverride.active = false;
        Serial.println("[OVERRIDE] Fertilizer pump override expired — returning to evaluator control.");
    }
}

void IrrigationController::_processRemoteCommands(const CropState& state, unsigned long now) {
    if (!_telemetry.hasPendingCommands()) {
        return;
    }

    const RemoteCommand* cmds = _telemetry.pendingCommands();
    uint8_t count = _telemetry.pendingCommandCount();

    for (uint8_t i = 0; i < count; ++i) {
        if (!cmds[i].valid) continue;

        const RemoteCommand& cmd = cmds[i];
        bool isWater = (strcmp(cmd.target, "water_pump") == 0);
        bool isFertilizer = (strcmp(cmd.target, "fertilizer_pump") == 0);

        if (!isWater && !isFertilizer) {
            _reportCommandResult(cmd.target, false, "UNKNOWN_TARGET");
            continue;
        }

        // Tier 1 safety: reject pump ON if tank is empty OR sensor is invalid (can't verify safety)
        bool wantsOn = (strcmp(cmd.state, "ON") == 0);
        if (wantsOn) {
            if (!state.waterLevel.isValid) {
                _reportCommandResult(cmd.target, false, "SAFETY_VIOLATION: water_sensor_invalid");
                Serial.println("[SAFETY] Remote pump command rejected — water level sensor invalid.");
                continue;
            }
            if (state.waterLevel.status == WaterLevelStatus::EMPTY) {
                _reportCommandResult(cmd.target, false, "SAFETY_VIOLATION: tank_empty");
                Serial.println("[SAFETY] Remote pump command rejected — tank is EMPTY.");
                continue;
            }
        }

        if (isWater) {
            if (wantsOn) {
                _waterOverride.active = true;
                _waterOverride.endTime = now + (cmd.durationSec * 1000UL);
                _reportCommandResult(cmd.target, true, "override_active");
                Serial.print("[OVERRIDE] Water pump ON for ");
                Serial.print(cmd.durationSec);
                Serial.println("s");
            } else {
                _waterOverride.active = false;
                _reportCommandResult(cmd.target, true, "override_cancelled");
                Serial.println("[OVERRIDE] Water pump OFF (user command)");
            }
        } else if (isFertilizer) {
            if (wantsOn) {
                _fertilizerOverride.active = true;
                _fertilizerOverride.endTime = now + (cmd.durationSec * 1000UL);
                _reportCommandResult(cmd.target, true, "override_active");
                Serial.print("[OVERRIDE] Fertilizer pump ON for ");
                Serial.print(cmd.durationSec);
                Serial.println("s");
            } else {
                _fertilizerOverride.active = false;
                _reportCommandResult(cmd.target, true, "override_cancelled");
                Serial.println("[OVERRIDE] Fertilizer pump OFF (user command)");
            }
        }
    }

    _telemetry.clearPendingCommands();
}

void IrrigationController::_reportCommandResult(const char* command, bool executed, const char* reason) {
    if (_cmdResultCount >= MAX_CMD_RESULTS) return;

    CommandResult& cr = _cmdResults[_cmdResultCount++];
    strlcpy(cr.command, command, sizeof(cr.command));
    cr.executed = executed;
    strlcpy(cr.reason, reason, sizeof(cr.reason));
    cr.valid = true;
}

void IrrigationController::_driveActuators(AgronomicDiagnosis& diagnosis, unsigned long now) {
    // Water pump: remote override active?
    if (_waterOverride.active && now < _waterOverride.endTime) {
        _waterPump.execute(Command::TURN_ON_WATER);
        diagnosis.requiresIrrigation = true; // reflect actual state in telemetry
    } else {
        _waterPump.execute(diagnosis.requiresIrrigation ? Command::TURN_ON_WATER : Command::TURN_OFF_WATER);
    }

    // Fertilizer pump: remote override active?
    if (_fertilizerOverride.active && now < _fertilizerOverride.endTime) {
        _fertilizerPump.execute(Command::TURN_ON_FERTILIZER);
        diagnosis.requiresFertilization = true; // reflect actual state in telemetry
    } else {
        _fertilizerPump.execute(diagnosis.requiresFertilization ? Command::TURN_ON_FERTILIZER : Command::TURN_OFF_FERTILIZER);
    }
}