#ifndef READINGS_H
#define READINGS_H

#include "Types.h"

struct SoilMoistureReading {
    float percentage;
    int rawValue; // ADC crudo (0–4095) para calibración y debugging
    bool isValid; // True si el sensor capacitivo está respondiendo correctamente
};

struct SoilFertilityReading {
    float conductivity;
    bool isValid; // True si el sensor resistivo YL-69 está conectado
};

struct SoilTemperatureReading {
    float celsius;
    bool isValid; // True si la sonda DS18B20 responde en el bus OneWire
};

struct EnvironmentReading {
    float temperature;
    float humidity;
    bool isValid; // True si el DHT22 no arroja NaN (Not a Number)
};

struct WaterLevelReading {
    WaterLevelStatus status;
    bool isValid; // True si los pines de la boya hacen buen contacto
};

struct CropState {
    SoilMoistureReading soilMoisture;
    SoilFertilityReading soilFertility;
    SoilTemperatureReading soilTemperature;
    EnvironmentReading environment;
    WaterLevelReading waterLevel;
};

struct AgronomicDiagnosis {
    bool requiresIrrigation;
    bool requiresFertilization;
    const char* alertMessage; 
};

#endif