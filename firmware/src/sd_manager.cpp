#include "sd_manager.h"
#include <esp_task_wdt.h>

SDManager::SDManager()
    : _spi(HSPI), _mounted(false) {
}

bool SDManager::begin(uint8_t cs, uint8_t mosi, uint8_t miso, uint8_t clk) {
    _mounted = false;

    Serial.printf("[SD] Inicializando SPI: CLK=%d, MISO=%d, MOSI=%d, CS=%d\n",
                  clk, miso, mosi, cs);

    // Inicializar bus SPI en los pines custom
    // En ESP32-S3 con PSRAM octal, usamos FSPI (SPI2) remapeado a pines libres
    _spi.begin(clk, miso, mosi, cs);

    // Intentar montar la SD con reintentos (algunas tarjetas tardan)
    // Usar 4MHz para compatibilidad maxima. La camara I2S DMA puede causar
    // TG1WDT si el CPU bloquea mucho tiempo en SPI a alta velocidad.
    for (uint8_t intento = 0; intento < 3; intento++) {
        if (SD.begin(cs, _spi, 4000000)) { // 4MHz SPI clock
            _mounted = true;
            Serial.printf("[SD] Montada OK en intento %d. Total: %llu MB, Libre: %llu MB\n",
                          intento + 1, getTotalSpaceMB(), getFreeSpaceMB());
            return true;
        }
        Serial.printf("[SD] Intento %d fallido, reintentando en 200ms...\n", intento + 1);
        delay(200);
        // Alimentar WDT durante reintentos para evitar reset
        esp_task_wdt_reset();
    }

    Serial.println("[SD] FALLO al montar despues de 3 intentos. Verificar conexiones.");
    return false;
}

bool SDManager::tryMount(uint8_t cs, uint8_t mosi, uint8_t miso, uint8_t clk) {
    if (_mounted) return true;

    _mounted = false;
    Serial.printf("[SD] tryMount: CLK=%d, MISO=%d, MOSI=%d, CS=%d\n",
                  clk, miso, mosi, cs);

    _spi.begin(clk, miso, mosi, cs);

    // Un solo intento, 4MHz, timeout corto.
    // La tarea debe estar FUERA del WDT antes de llamar a esta funcion,
    // porque SD.begin() bloquea ~0.9s sin tarjeta y el SPI DMA usa TG1.
    if (SD.begin(cs, _spi, 4000000, "/sd", 1)) {
        _mounted = true;
        Serial.printf("[SD] Montada OK. Total: %llu MB, Libre: %llu MB\n",
                      getTotalSpaceMB(), getFreeSpaceMB());
        return true;
    }

    Serial.println("[SD] Sin tarjeta detectada.");
    return false;
}

bool SDManager::isMounted() const {
    return _mounted;
}

uint64_t SDManager::getFreeSpaceMB() {
    if (!_mounted) return 0;
    return (SD.totalBytes() - SD.usedBytes()) / (1024ULL * 1024ULL);
}

uint64_t SDManager::getTotalSpaceMB() {
    if (!_mounted) return 0;
    return SD.totalBytes() / (1024ULL * 1024ULL);
}

std::vector<String> SDManager::listFiles(const char* path) {
    std::vector<String> files;
    if (!_mounted) return files;

    listDirRecursive(SD, path, files);
    return files;
}

bool SDManager::writeFile(const char* path, const uint8_t* data, size_t len) {
    if (!_mounted) return false;

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[SD] Error abriendo %s para escritura\n", path);
        return false;
    }

    size_t written = file.write(data, len);
    file.close();

    if (written != len) {
        Serial.printf("[SD] Error escritura %s: esperaba %u, escritos %u\n", path, len, written);
        return false;
    }
    return true;
}

bool SDManager::appendFile(const char* path, const uint8_t* data, size_t len) {
    if (!_mounted) return false;

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
        Serial.printf("[SD] Error abriendo %s para anexar\n", path);
        return false;
    }

    size_t written = file.write(data, len);
    file.close();

    return (written == len);
}

bool SDManager::deleteFile(const char* path) {
    if (!_mounted) return false;

    if (!SD.exists(path)) {
        Serial.printf("[SD] No existe: %s\n", path);
        return false;
    }

    bool ok = SD.remove(path);
    if (ok) {
        Serial.printf("[SD] Eliminado: %s\n", path);
    } else {
        Serial.printf("[SD] Error eliminando: %s\n", path);
    }
    return ok;
}

bool SDManager::mkdir(const char* path) {
    if (!_mounted) return false;
    return SD.mkdir(path);
}

bool SDManager::exists(const char* path) {
    if (!_mounted) return false;
    return SD.exists(path);
}

size_t SDManager::readFile(const char* path, uint8_t* buffer, size_t maxLen) {
    if (!_mounted) return 0;

    File file = SD.open(path, FILE_READ);
    if (!file) {
        Serial.printf("[SD] Error abriendo %s para lectura\n", path);
        return 0;
    }

    size_t toRead = min(maxLen, (size_t)file.size());
    size_t bytesRead = file.read(buffer, toRead);
    file.close();

    return bytesRead;
}

bool SDManager::renameFile(const char* oldPath, const char* newPath) {
    if (!_mounted) return false;

    if (!SD.exists(oldPath)) {
        Serial.printf("[SD] Origen no existe: %s\n", oldPath);
        return false;
    }

    bool ok = SD.rename(oldPath, newPath);
    if (ok) {
        Serial.printf("[SD] Renombrado: %s -> %s\n", oldPath, newPath);
    } else {
        Serial.printf("[SD] Error renombrando: %s -> %s\n", oldPath, newPath);
    }
    return ok;
}

void SDManager::listDirRecursive(fs::FS &fs, const char* dirname, std::vector<String> &files) {
    File root = fs.open(dirname, FILE_READ);
    if (!root) return;
    if (!root.isDirectory()) {
        root.close();
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            // Recursion en subdirectorio
            listDirRecursive(fs, file.path(), files);
        } else {
            // Construir ruta completa: dirname + "/" + nombre_archivo
            String fullPath = String(dirname);
            if (!fullPath.endsWith("/")) fullPath += "/";
            fullPath += file.name();
            files.push_back(fullPath);
        }
        file = root.openNextFile();
    }
    root.close();
}
