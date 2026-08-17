#include "servo_manager.h"

ServoManager::ServoManager()
    : _pin(0), _channel(0), _currentAngle(90), _initialized(false) {
}

bool ServoManager::begin(uint8_t pin, uint8_t channel) {
    _pin = pin;
    _channel = channel;
    _currentAngle = 90;

    // Configurar canal LEDC: 50Hz, 14 bits (maximo ESP32-S3)
    uint32_t result = ledcSetup(_channel, PWM_FREQ_HZ, PWM_RESOLUTION);
    if (result == 0) {
        Serial.printf("[SERVO] FALLO ledcSetup canal %d (%dHz, %dbits)\n",
                      _channel, PWM_FREQ_HZ, PWM_RESOLUTION);
        _initialized = false;
        return false;
    }

    // Asignar pin al canal LEDC
    ledcAttachPin(_pin, _channel);

    // Mover a posicion central por defecto
    ledcWrite(_channel, angleToDuty(_currentAngle));

    _initialized = true;
    Serial.printf("[SERVO] Inicializado: pin=%d, canal=%d, angulo=%d\n",
                  _pin, _channel, _currentAngle);
    return true;
}

void ServoManager::setAngle(uint8_t angle) {
    if (!_initialized) return;

    // Recortar a rango valido 0-180
    angle = constrain(angle, (uint8_t)0, (uint8_t)180);

    _currentAngle = angle;
    ledcWrite(_channel, angleToDuty(angle));
}

uint8_t ServoManager::getAngle() const {
    return _currentAngle;
}

void ServoManager::sleep() {
    if (!_initialized) return;

    // Detener la senal PWM. El servo pierde energia pero mantiene
    // posicion por friccion mecanica de los engranajes (SG90/MG90S).
    // Funciona bien con carga ligera (camara, lente pequena).
    ledcDetachPin(_pin);

    Serial.printf("[SERVO] Sleep: PWM detenido en GPIO%d, angulo=%d guardado\n",
                  _pin, _currentAngle);
}

void ServoManager::wake() {
    if (!_initialized) return;

    // Restaurar el canal LEDC y re-aplicar el ultimo angulo
    ledcAttachPin(_pin, _channel);
    ledcWrite(_channel, angleToDuty(_currentAngle));

    Serial.printf("[SERVO] Wake: PWM restaurado, angulo=%d\n", _currentAngle);
}

void ServoManager::detach() {
    if (!_initialized) return;

    ledcDetachPin(_pin);
    _initialized = false;

    Serial.printf("[SERVO] Desconectado: pin=%d liberado\n", _pin);
}

uint16_t ServoManager::angleToDuty(uint8_t angle) const {
    // Mapeo lineal entero: 0° -> 1638, 180° -> 8191
    // duty = (angle * (DUTY_MAX - DUTY_MIN) / 180) + DUTY_MIN
    uint32_t range = (uint32_t)(DUTY_MAX - DUTY_MIN);
    uint32_t duty = ((uint32_t)angle * range) / 180UL + (uint32_t)DUTY_MIN;
    return (uint16_t)constrain(duty, (uint32_t)DUTY_MIN, (uint32_t)DUTY_MAX);
}
