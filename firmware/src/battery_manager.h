#ifndef BATTERY_MANAGER_H
#define BATTERY_MANAGER_H

#include <Arduino.h>

class BatteryMonitor {
public:
    BatteryMonitor();

    // Inicializa el monitor de bateria en el pin ADC especificado.
    // Detecta automaticamente si hay hardware conectado (plug-and-play).
    // Retorna true si la inicializacion fue exitosa Y hay hardware detectado.
    bool begin(uint8_t pin, float dividerRatio = 2.0f);

    // Retorna true si se detecto hardware de bateria conectado.
    bool isPresent() const;

    // Lee el voltaje real de la bateria (ej. 3.85V).
    // Promedia SAMPLES lecturas para estabilidad.
    float readVoltage();

    // Retorna porcentaje 0-100% mapeado entre 3.0V (0%) y 4.2V (100%).
    uint8_t getPercentage();

    // Ajuste de calibracion: offset en voltajes para compensar con multímetro.
    void setCalibration(float offsetV);

    // Retorna ultimo voltaje leido sin volver a medir.
    float getLastVoltage() const;

    // Retorna ultimo porcentaje leido sin volver a medir.
    uint8_t getLastPercentage() const;

private:
    uint8_t _pin;
    float _divider;
    float _offset;
    float _lastVoltage;
    uint8_t _lastPercentage;
    bool _initialized;
    bool _present;

    static const uint8_t SAMPLES = 10;
    static const unsigned long SAMPLE_DELAY_MS = 5;
};

#endif // BATTERY_MANAGER_H
