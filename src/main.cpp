#include <Arduino.h>
#include "IrrigationController.h"

// ==========================================
// CONFIGURACIÓN DE ASIGNACIÓN DE PINES (GPIO)
// ==========================================
// Pines de Sensores
const uint8_t PIN_MOISTURE    = 32; // ADC1
const uint8_t PIN_FERTILITY   = 34; // ADC1
const uint8_t PIN_TEMP_SUELO  = 25; // Bus OneWire digital
const uint8_t PIN_DHT22       = 26; // Digital DHT
const uint8_t PIN_WATER_LEVEL = 27; // Entrada Digital Boya

// Pines de Actuadores (Relés de Bombas)
const uint8_t PIN_WATER_PUMP      = 14; // Salida digital — seguro, no es strapping pin
const uint8_t PIN_FERTILIZER_PUMP = 13; // Salida digital — seguro, no es strapping pin

// Instanciación única del Orquestador Global
IrrigationController controller(
    PIN_MOISTURE, PIN_FERTILITY, PIN_TEMP_SUELO, PIN_DHT22, PIN_WATER_LEVEL,
    PIN_WATER_PUMP, PIN_FERTILIZER_PUMP
);

void setup() {
    // Abrimos consola serial para debugear
    Serial.begin(115200);
    delay(1000);
    Serial.println("[SYSTEM START] Inicializando AquaEdge Core...");
    
    // Inicializa todo el pipeline de componentes
    controller.begin();
    
    Serial.println("[SYSTEM READY] Orquestador corriendo de forma asíncrona.");
}

void loop() {
    // El método tick se ejecuta a máxima velocidad revisando los deltas de tiempo
    controller.tick();
    
    // Aquí puedes meter otras tareas pesadas en el futuro sin que congelen el riego
}