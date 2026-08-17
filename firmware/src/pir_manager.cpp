#include "pir_manager.h"
#include "config.h"

PIRManager::PIRManager()
    : _pin(0), _callback(nullptr), _triggered(false),
      _enabled(false), _initialized(false), _present(false),
      _lastTriggerTime(0), _callbackPending(false) {
}

bool PIRManager::begin(uint8_t pin, TriggerCallback callback) {
    _pin = pin;
    _callback = callback;
    _triggered = false;
    _enabled = false;
    _callbackPending = false;
    _lastTriggerTime = 0;
    _present = false;

    // DETECCION PLUG-AND-PLAY:
    // El HC-SR501 tiene salida LOW por defecto, HIGH al detectar movimiento.
    // Si el pin esta desconectado, flotara entre HIGH y LOW aleatoriamente.
    // Leemos varias veces para detectar flote.
    pinMode(_pin, INPUT_PULLDOWN);
    delay(50);
    uint8_t lowCount = 0;
    for (uint8_t i = 0; i < HW_PIR_DETECTION_SAMPLES; i++) {
        if (digitalRead(_pin) == LOW) lowCount++;
        delay(50);
    }

    // Estrategia: si el pin leyo LOW consistentemente, puede ser:
    //   a) PIR conectado sin movimiento (normalmente LOW) -> presente
    //   b) Pin desconectado con pull-down interno -> no presente
    // Si leyo HIGH al menos una vez, definitivamente hay algo conectado.
    // Para v1: asumir presente siempre que el pin sea valido para interrupciones.
    // El usuario puede desconectar fisicamente y el sistema simplemente no
    // recibira eventos PIR -> forzara modo grabacion continua.
    // "Presente" se reporta como true; en la practica, la ausencia real
    // se detecta por timeout de eventos en la capa de aplicacion.

    _present = true;  // Asumimos presente; la ausencia se maneja por timeout

    pinMode(_pin, INPUT);  // Quitar pull-down, el PIR maneja su propio estado
    attachInterruptArg(digitalPinToInterrupt(_pin), onInterrupt, this, RISING);

    _initialized = true;
    _enabled = true;

    Serial.printf("[PIR] Inicializado: pin=%d. Si no hay sensor conectado, "
                  "el sistema operara en modo grabacion continua.\n", _pin);
    return true;
}

bool PIRManager::isPresent() const {
    return _present;
}

void PIRManager::enable() {
    if (!_initialized) return;

    if (!_enabled) {
        attachInterruptArg(digitalPinToInterrupt(_pin), onInterrupt, this, RISING);
        _enabled = true;
        Serial.printf("[PIR] Habilitado en pin %d\n", _pin);
    }
}

void PIRManager::disable() {
    if (!_initialized) return;

    if (_enabled) {
        detachInterrupt(digitalPinToInterrupt(_pin));
        _enabled = false;
        _triggered = false;
        _callbackPending = false;
        Serial.printf("[PIR] Deshabilitado en pin %d\n", _pin);
    }
}

bool PIRManager::isTriggered() const {
    return _triggered;
}

void PIRManager::update() {
    if (!_initialized || !_enabled) return;

    unsigned long now = millis();

    // Procesar callback pendiente con debounce
    if (_callbackPending) {
        if ((now - _lastTriggerTime) >= DEBOUNCE_MS) {
            _callbackPending = false;
            _lastTriggerTime = now;

            // Llamar al callback si esta configurado
            if (_callback) {
                _callback();
            }
        }
    }

    // Resetear flag _triggered despues de TRIGGER_HOLD_MS
    // Esto permite que isTriggered() retorne true por un tiempo razonable
    if (_triggered && !_callbackPending) {
        if ((now - _lastTriggerTime) > TRIGGER_HOLD_MS) {
            _triggered = false;
        }
    }
}

void IRAM_ATTR PIRManager::onInterrupt(void* arg) {
    // ISR: ejecuta en RAM interna para baja latencia
    // Solo setea flags, NO ejecuta codigo pesado aqui
    PIRManager* self = static_cast<PIRManager*>(arg);
    if (self->_enabled) {
        self->_triggered = true;
        self->_callbackPending = true;
    }
}
