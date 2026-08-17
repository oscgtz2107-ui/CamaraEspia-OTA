#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

class CameraManager {
public:
    CameraManager();

    // Inicializa la camara OV3660 con los pines definidos en config.h.
    // Usa PSRAM para buffers de frame.
    // Retorna true si la camara se inicializo correctamente.
    bool begin();

    // Captura un frame JPEG y lo devuelve en un buffer.
    // El caller debe liberar el buffer con free() cuando termine.
    // Retorna nullptr si falla.
    // El tamano del buffer se escribe en *outLen.
    uint8_t* captureJPEG(size_t* outLen);

    // Captura un frame JPEG en el buffer proporcionado.
    // maxLen: tamano maximo del buffer.
    // Retorna true si cabe y se copio correctamente.
    bool captureJPEGToBuffer(uint8_t* buffer, size_t maxLen, size_t* outLen);

    // Cambia la resolucion en runtime.
    // VGA=640x480, HD=1280x720, 3MP=1600x1200 (UXGA fallback).
    bool setResolution(Resolution res);

    // Retorna la resolucion actual.
    Resolution getResolution() const;

    // Entra en modo standby (bajo consumo, mantiene sensor alimentado).
    // El wake es instantaneo (~50ms).
    bool standby();

    // Sale de standby y vuelve a modo activo.
    bool wake();

    // Desinicializa COMPLETAMENTE la camara (libera I2S DMA, Timer Group).
    // Necesario antes de operaciones largas en SPI que bloquean el CPU.
    bool deinit();

    // Reinicializa la camara despues de deinit(). Reconfigura sensor.
    bool reinit();

    // Verifica si la camara esta inicializada y lista.
    bool isInitialized() const;

    // Verifica si el sensor esta en standby (VSYNC detenido).
    bool isStandby() const;

    // Retorna el tamano estimado de un frame JPEG en la resolucion actual.
    // Util para pre-asignar buffers.
    size_t getEstimatedFrameSize() const;

private:
    bool _initialized;
    bool _standby;
    Resolution _currentResolution;
    framesize_t _frameSize;
    camera_config_t _config;  // Guardado para reinit despues de deinit()

    framesize_t resolutionToFrameSize(Resolution res) const;
    bool configureSensor();
};

#endif // CAMERA_MANAGER_H
