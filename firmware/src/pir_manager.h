#ifndef PIR_MANAGER_H
#define PIR_MANAGER_H

#include <Arduino.h>
#include <functional>

class PIRManager {
public:
    // Tipo de callback: se invoca cuando se detecta movimiento
    using TriggerCallback = std::function<void()>;

    PIRManager();

    // Inicializa el sensor PIR en el pin especificado.
    // Detecta automaticamente si hay hardware conectado (plug-and-play).
    // Retorna true si la inicializacion fue exitosa.
    bool begin(uint8_t pin, TriggerCallback callback = nullptr);

    // Retorna true si se detecto sensor PIR conectado.
    bool isPresent() const;

    // Habilita la deteccion de movimiento (adjunta interrupcion).
    void enable();

    // Deshabilita la deteccion (desadjunta interrupcion).
    void disable();

    // Retorna true si hay movimiento detectado actualmente.
    bool isTriggered() const;

    // Debe llamarse en loop() para manejar debounce y disparar callbacks.
    // Sin esta llamada, el callback NO se ejecuta.
    void update();

private:
    uint8_t _pin;
    TriggerCallback _callback;
    volatile bool _triggered;
    bool _enabled;
    bool _initialized;
    bool _present;
    unsigned long _lastTriggerTime;
    bool _callbackPending;

    // Debounce minimo entre triggers (evita multiples callbacks por un solo movimiento)
    static const unsigned long DEBOUNCE_MS = 300;

    // Duracion del flag _triggered antes de resetearse automaticamente
    static const unsigned long TRIGGER_HOLD_MS = 2000;

    // ISR: interrupcion por flanco de subida (HIGH = movimiento detectado)
    static void IRAM_ATTR onInterrupt(void* arg);
};

#endif // PIR_MANAGER_H
