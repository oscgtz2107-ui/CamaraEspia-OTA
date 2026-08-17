#include "battery_manager.h"
#include "config.h"

BatteryMonitor::BatteryMonitor()
    : _pin(0), _divider(2.0f), _offset(0.0f),
      _lastVoltage(0.0f), _lastPercentage(0),
      _initialized(false), _present(false) {
}

bool BatteryMonitor::begin(uint8_t pin, float dividerRatio) {
    _pin = pin;
    _divider = dividerRatio;
    _offset = 0.0f;
    _present = false;
    _initialized = false;

    // Configurar ADC: atenuacion 11dB para rango 0-3.3V en el pin
    analogSetAttenuation(ADC_11db);
    analogReadResolution(12); // 12 bits: 0-4095

    // DETECCION PLUG-AND-PLAY:
    // Leer ADC varias veces. Si el valor esta entre HW_DETECTION_ADC_MIN y MAX,
    // asumir que hay un divisor conectado. Si es 0 o 4095, el pin esta flotante.
    uint32_t sum = 0;
    for (uint8_t i = 0; i < HW_DETECTION_SAMPLES; i++) {
        sum += analogRead(_pin);
        delay(10);
    }
    uint16_t avg = sum / HW_DETECTION_SAMPLES;

    if (avg < HW_DETECTION_ADC_MIN || avg > HW_DETECTION_ADC_MAX) {
        Serial.printf("[BAT] NO DETECTADO (ADC=%d, fuera de rango %d-%d). "
                      "Conecta el divisor de voltaje si deseas monitoreo de bateria.\n",
                      avg, HW_DETECTION_ADC_MIN, HW_DETECTION_ADC_MAX);
        _initialized = false;
        _present = false;
        return false;  // No es fatal, el sistema sigue funcionando
    }

    _present = true;
    _initialized = true;

    // Lectura inicial para tener valores validos desde el start
    readVoltage();

    Serial.printf("[BAT] Detectado: pin=%d, divisor=%.1f, voltaje=%.2fV (%d%%)\n",
                  _pin, _divider, _lastVoltage, _lastPercentage);
    return true;
}

bool BatteryMonitor::isPresent() const {
    return _present;
}

float BatteryMonitor::readVoltage() {
    if (!_initialized || !_present) return 0.0f;

    // Promediar SAMPLES lecturas para reducir ruido
    uint32_t sum = 0;
    for (uint8_t i = 0; i < SAMPLES; i++) {
        sum += analogRead(_pin);
        delay(SAMPLE_DELAY_MS);
    }

    float avgADC = (float)sum / (float)SAMPLES;

    // Convertir ADC a voltaje en el pin
    // Con ADC_11db: rango 0-3.3V, resolucion 12 bits (0-4095)
    float vPin = (avgADC / 4095.0f) * 3.3f;

    // Aplicar divisor de voltaje para obtener V_bateria real
    _lastVoltage = (vPin * _divider) + _offset;

    // Calcular porcentaje: 3.0V = 0%, 4.2V = 100%
    float pct = ((_lastVoltage - BATT_VOLT_MIN) / (BATT_VOLT_MAX - BATT_VOLT_MIN)) * 100.0f;
    _lastPercentage = (uint8_t)constrain((int)pct, 0, 100);

    return _lastVoltage;
}

uint8_t BatteryMonitor::getPercentage() {
    return _lastPercentage;
}

void BatteryMonitor::setCalibration(float offsetV) {
    _offset = offsetV;
    Serial.printf("[BAT] Calibracion: offset = %.3fV\n", _offset);
}

float BatteryMonitor::getLastVoltage() const {
    return _lastVoltage;
}

uint8_t BatteryMonitor::getLastPercentage() const {
    return _lastPercentage;
}
