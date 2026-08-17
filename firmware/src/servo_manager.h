#ifndef SERVO_MANAGER_H
#define SERVO_MANAGER_H

#include <Arduino.h>

class ServoManager {
public:
    ServoManager();

    // Inicializa LEDC para control del servo SG90/MG90S.
    // pin: GPIO de senal PWM. channel: canal LEDC (0-15).
    // Retorna true si la inicializacion fue exitosa.
    bool begin(uint8_t pin, uint8_t channel = 0);

    // Mueve servo a angulo (0-180). Valores fuera de rango se recortan.
    void setAngle(uint8_t angle);

    // Retorna angulo actual configurado.
    uint8_t getAngle() const;

    // Detiene LEDC (señal PWM cortada). El servo mantiene posicion
    // por friccion mecanica de los engranajes. Para Light Sleep.
    void sleep();

    // Restaura LEDC con el ultimo angulo registrado.
    // Llamar al despertar de Light Sleep.
    void wake();

    // Libera completamente el canal LEDC.
    void detach();

private:
    uint8_t _pin;
    uint8_t _channel;
    uint8_t _currentAngle;
    bool _initialized;

    // Parametros PWM fijos para servo estandar
    static const uint16_t PWM_FREQ_HZ = 50;  // 50Hz = periodo 20ms
    static const uint8_t PWM_RESOLUTION = 14; // 14 bits: 0-16383 (maximo ESP32-S3)

    // Rangos de duty cycle calculados (14 bits = 16383):
    // 0°   = 0.5ms pulso / 20ms periodo = 2.5%  -> 0.025 * 16383 = 410
    // 180° = 2.5ms pulso / 20ms periodo = 12.5% -> 0.125 * 16383 = 2048
    static const uint16_t DUTY_MIN = 410;   // 0°
    static const uint16_t DUTY_MAX = 2048;  // 180°

    // Convierte angulo (0-180) a duty cycle (1638-8191)
    uint16_t angleToDuty(uint8_t angle) const;
};

#endif // SERVO_MANAGER_H
