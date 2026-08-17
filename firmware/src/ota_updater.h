#ifndef OTA_UPDATER_H
#define OTA_UPDATER_H

#include <Arduino.h>
#include "config.h"
#include <Update.h>

class OTAUpdater {
public:
    OTAUpdater();

    // Inicializa modulo OTA
    void begin();

    // Descarga firmware desde URL y flashea
    bool updateFromURL(const char* url);

    // Verifica hash SHA256 de datos recibidos
    bool verifySHA256(const uint8_t* data, size_t len, const char* expected) const;

    // Rollback a particion anterior (OTA_0)
    bool rollback();

    // Retorna true si una actualizacion esta en progreso
    bool isUpdating() const;

private:
    bool updating;
    bool updateSuccess;
    int currentPartition;  // 0 = ota_0, 1 = ota_1

    // Descarga binario desde URL
    bool downloadAndFlash(const char* url);

    // Cambia particion activa en OTA
    bool switchPartition(int target);

    // Calcula SHA256 de un buffer
    String computeSHA256(const uint8_t* data, size_t len) const;
};

#endif // OTA_UPDATER_H
