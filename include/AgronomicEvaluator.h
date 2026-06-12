#ifndef AGRONOMIC_EVALUATOR_H
#define AGRONOMIC_EVALUATOR_H

#include "Readings.h"

class AgronomicEvaluator {
private:
    // Umbrales de negocio (Reglas agronómicas fijas)
    const float MOISTURE_THRESHOLD = 30.0f;     // Suelo seco si es menor al 30%
    const float FERTILITY_THRESHOLD = 1.5f;    // Nutrientes bajos si la CE es menor a 1.5 mS/cm

public:
    // Constructor por defecto
    AgronomicEvaluator();

    // Método principal que recibe el estado inmutable y calcula el diagnóstico
    AgronomicDiagnosis evaluate(const CropState& state);
};

#endif // AGRONOMIC_EVALUATOR_H