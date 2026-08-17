#include "power_manager.h"

PowerManager::PowerManager()
    : wdtFlag(false)
    , bootTime(0)
    , lastWakeSource(WAKE_NONE) {
    // TODO: inicializar miembros
}

void PowerManager::begin() {
    // TODO: prefs.begin("camara_espia", false)
    // TODO: bootTime = millis()
    // TODO: verificar readRTCFlag() == WDT_SAFE_BOOT_VALUE
}

void PowerManager::enterLightSleep() {
    // TODO: configurar wake sources (GPIO PIR + boton)
    // TODO: esp_light_sleep_start()
    // TODO: detectar wake source al despertar
}

bool PowerManager::wasWdtReset() const {
    return wdtFlag;
}

void PowerManager::setSafeBootFlag() {
    // TODO: writeRTCFlag(WDT_SAFE_BOOT_VALUE)
}

void PowerManager::clearSafeBootFlag() {
    // TODO: writeRTCFlag(0)
}

bool PowerManager::isDiagMode() const {
    // TODO: verificar si PIN_BOOT esta LOW por > 5s al inicio
    return false;
}

PowerManager::WakeSource PowerManager::getSleepWakeSource() const {
    return lastWakeSource;
}

uint32_t PowerManager::readRTCFlag() const {
    // TODO: leer de RTC_DATA_ATTR variable
    return 0;
}

void PowerManager::writeRTCFlag(uint32_t value) {
    // TODO: escribir a RTC_DATA_ATTR variable
}
