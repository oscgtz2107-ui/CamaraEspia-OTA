#ifndef SD_MANAGER_H
#define SD_MANAGER_H

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

class SDManager {
public:
    SDManager();

    // Inicializa la tarjeta SD con pines SPI custom.
    // Retorna true si la SD se mont correctamente.
    bool begin(uint8_t cs, uint8_t mosi, uint8_t miso, uint8_t clk);

    // Intento unico de montar la SD (sin reintentos, sin delays largos).
    // Retorna true si la SD se mont correctamente.
    // IMPORTANTE: el caller debe quitar la tarea del WDT antes de llamar.
    bool tryMount(uint8_t cs, uint8_t mosi, uint8_t miso, uint8_t clk);

    // Verifica si la SD esta montada y operativa.
    bool isMounted() const;

    // Retorna espacio libre en MB.
    uint64_t getFreeSpaceMB();

    // Retorna espacio total en MB.
    uint64_t getTotalSpaceMB();

    // Lista archivos recursivamente en un directorio.
    std::vector<String> listFiles(const char* path = "/");

    // Escribe datos en archivo (sobrescribe contenido existente).
    bool writeFile(const char* path, const uint8_t* data, size_t len);

    // Anexa datos al final del archivo.
    bool appendFile(const char* path, const uint8_t* data, size_t len);

    // Elimina un archivo.
    bool deleteFile(const char* path);

    // Crea un directorio.
    bool mkdir(const char* path);

    // Verifica si un archivo o directorio existe.
    bool exists(const char* path);

    // Lee archivo completo a buffer. Retorna bytes leidos.
    size_t readFile(const char* path, uint8_t* buffer, size_t maxLen);

    // Renombra un archivo.
    bool renameFile(const char* oldPath, const char* newPath);

private:
    SPIClass _spi;
    bool _mounted;

    // Listado recursivo interno
    void listDirRecursive(fs::FS &fs, const char* dirname, std::vector<String> &files);
};

#endif // SD_MANAGER_H
