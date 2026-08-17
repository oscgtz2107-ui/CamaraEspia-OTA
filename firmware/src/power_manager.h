#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include <Preferences.h>

class PowerManager {
public:
    // Wake sources posibles (public para que otros modulos lo usen)
    enum WakeSource {
        WAKE_NONE,
        WAKE_PIR,
        WAKE_BUTTON
    };

    PowerManager();

    // Inicializa NVS y verifica flags de boot
    void begin();

    // Configura wake sources y entra en light sleep
    void enterLightSleep();

    // Verifica si el reset fue causado por watchdog
    bool wasWdtReset() const;

    // Marca flag de boot seguro en RTC memory
    void setSafeBootFlag();

    // Limpia flag de boot seguro
    void clearSafeBootFlag();

    // Verifica si el boton BOOT esta presionado > 5s
    bool isDiagMode() const;

    // Retorna fuente de wake (PIR/BUTTON/NONE)
    WakeSource getSleepWakeSource() const;

private:
    Preferences prefs;
    bool wdtFlag;
    uint32_t bootTime;

    WakeSource lastWakeSource;

    // Lee flag de RTC memory
    uint32_t readRTCFlag() const;

    // Escribe flag en RTC memory
    void writeRTCFlag(uint32_t value);
};

#endif // POWER_MANAGER_H
