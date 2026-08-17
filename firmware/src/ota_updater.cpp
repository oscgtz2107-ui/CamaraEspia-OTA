#include "ota_updater.h"

OTAUpdater::OTAUpdater()
    : updating(false)
    , updateSuccess(false)
    , currentPartition(0) {
    // TODO: inicializar miembros
}

void OTAUpdater::begin() {
    // TODO: determinar particion actual (ota_0 o ota_1)
    // TODO: configurar Update con callbacks de progreso
}

bool OTAUpdater::updateFromURL(const char* url) {
    // TODO: validar URL (max OTA_URL_MAX_SIZE)
    // TODO: downloadAndFlash(url)
    // TODO: si OK, switchPartition a la otra
    updating = true;
    return false;
}

bool OTAUpdater::verifySHA256(const uint8_t* data, size_t len, const char* expected) const {
    // TODO: calcular SHA256 de data
    // TODO: comparar con expected
    return false;
}

bool OTAUpdater::rollback() {
    // TODO: volver a particion anterior
    // TODO: esp_ota_mark_app_valid_cancel_rollback()
    return false;
}

bool OTAUpdater::isUpdating() const {
    return updating;
}

bool OTAUpdater::downloadAndFlash(const char* url) {
    // TODO: HTTP GET del .bin
    // TODO: Update.begin(contentLength)
    // TODO: write chunks while downloading
    // TODO: Update.end(true)
    return false;
}

bool OTAUpdater::switchPartition(int target) {
    // TODO: esp_ota_set_boot_partition() segun target (0=ota_0, 1=ota_1)
    return false;
}

String OTAUpdater::computeSHA256(const uint8_t* data, size_t len) const {
    // TODO: usar mbedTLS sha256
    return String("");
}
