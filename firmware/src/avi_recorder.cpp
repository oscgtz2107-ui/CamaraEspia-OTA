#include "avi_recorder.h"

uint16_t AVIRecorder::_fileCounter = 0;

AVIRecorder::AVIRecorder()
    : _sd(nullptr), _camera(nullptr), _recording(false),
      _tempFile(""), _finalFile(""),
      _startTime(0), _frameCount(0), _moviSize(0), _important(false),
      _offsetFileSize(0), _offsetTotalFrames(0), _offsetLength(0), _offsetMoviSize(0),
      _preBuffer(nullptr), _preBufferSize(0), _preBufferHead(0),
      _preBufferCount(0), _preBufferActive(false) {
}

bool AVIRecorder::begin(SDManager* sd, CameraManager* cam) {
    _sd = sd;
    _camera = cam;
    _recording = false;
    _preBuffer = nullptr;
    _preBufferActive = false;

    // Limpiar archivos .tmp huérfanos de reinicios anteriores
    if (_sd && _sd->isMounted()) {
        auto files = _sd->listFiles("/");
        for (const String& f : files) {
            if (f.endsWith(".tmp")) {
                Serial.printf("[AVI] Limpiando .tmp huérfano: %s\n", f.c_str());
                _sd->deleteFile(f.c_str());
            }
        }
    }

    Serial.println("[AVI] Inicializado");
    return true;
}

// ===== GRABACION =====

bool AVIRecorder::startRecording(const char* filename) {
    if (_recording) {
        Serial.println("[AVI] Ya grabando, ignorando startRecording");
        return false;
    }

    if (!_sd || !_sd->isMounted()) {
        Serial.println("[AVI] SD no disponible");
        return false;
    }

    _tempFile = String(filename) + ".tmp";
    _finalFile = String(filename);

    // Crear directorio si no existe
    int lastSlash = _finalFile.lastIndexOf('/');
    if (lastSlash > 0) {
        String dir = _finalFile.substring(0, lastSlash);
        if (!_sd->exists(dir.c_str())) {
            _sd->mkdir(dir.c_str());
        }
    }

    // Eliminar archivos previos
    if (_sd->exists(_tempFile.c_str())) {
        _sd->deleteFile(_tempFile.c_str());
    }
    if (_sd->exists(_finalFile.c_str())) {
        _sd->deleteFile(_finalFile.c_str());
    }

    // Abrir archivo temporal
    _file = SD.open(_tempFile.c_str(), FILE_WRITE);
    if (!_file) {
        Serial.printf("[AVI] No se pudo crear: %s\n", _tempFile.c_str());
        return false;
    }

    // Obtener dimensiones actuales de la cámara
    sensor_t* s = esp_camera_sensor_get();
    uint16_t width = 640;
    uint16_t height = 480;
    if (s) {
        width = s->id.PID == OV3660_PID ? 640 : 640;
        height = 480;
    }

    if (!writeAVIHeader(width, height)) {
        _file.close();
        _sd->deleteFile(_tempFile.c_str());
        return false;
    }

    _recording = true;
    _frameCount = 0;
    _moviSize = 0;
    _startTime = millis();
    _important = false;

    // Volcar pre-buffer si está activo
    if (_preBufferActive && _preBufferCount > 0) {
        flushPreBuffer();
    }

    Serial.printf("[AVI] Grabando: %s\n", _finalFile.c_str());
    return true;
}

bool AVIRecorder::stopRecording() {
    if (!_recording) return false;

    finalizeAVI();

    if (_file) {
        _file.close();
    }

    // Renombrar .tmp -> .avi
    if (_sd) {
        if (_sd->exists(_tempFile.c_str())) {
            _sd->renameFile(_tempFile.c_str(), _finalFile.c_str());
        }
    }

    _recording = false;

    Serial.printf("[AVI] Detenido: %u frames, %u bytes\n",
                  (unsigned)_frameCount, (unsigned)_moviSize);
    return true;
}

bool AVIRecorder::writeFrame() {
    if (!_recording || !_camera) return false;

    size_t len = 0;
    uint8_t* buf = _camera->captureJPEG(&len);
    if (!buf || len == 0) return false;

    bool ok = writeFrameChunk(buf, len);
    free(buf);

    if (ok) {
        _frameCount++;
    }
    return ok;
}

bool AVIRecorder::isRecording() const {
    return _recording;
}

// ===== BUFFER CIRCULAR =====

bool AVIRecorder::startPreBuffer(uint8_t maxFrames) {
    stopPreBuffer();

    _preBufferSize = maxFrames;
    _preBuffer = (PreCaptureFrame*)ps_malloc(sizeof(PreCaptureFrame) * maxFrames);
    if (!_preBuffer) {
        _preBuffer = (PreCaptureFrame*)malloc(sizeof(PreCaptureFrame) * maxFrames);
    }
    if (!_preBuffer) {
        Serial.printf("[AVI] Sin memoria para pre-buffer (%d frames)\n", maxFrames);
        return false;
    }

    for (uint8_t i = 0; i < _preBufferSize; i++) {
        _preBuffer[i].data = nullptr;
        _preBuffer[i].len = 0;
    }

    _preBufferHead = 0;
    _preBufferCount = 0;
    _preBufferActive = true;

    Serial.printf("[AVI] Pre-buffer activo: %d frames en PSRAM\n", maxFrames);
    return true;
}

void AVIRecorder::stopPreBuffer() {
    if (_preBuffer) {
        for (uint8_t i = 0; i < _preBufferSize; i++) {
            freePreBufferSlot(i);
        }
        free(_preBuffer);
        _preBuffer = nullptr;
    }
    _preBufferActive = false;
    _preBufferCount = 0;
    _preBufferHead = 0;
    _preBufferSize = 0;
}

void AVIRecorder::pushPreBuffer() {
    if (!_preBufferActive || !_camera) return;

    size_t len = 0;
    uint8_t* buf = _camera->captureJPEG(&len);
    if (!buf || len == 0) return;

    // Liberar slot anterior si tiene datos
    freePreBufferSlot(_preBufferHead);

    _preBuffer[_preBufferHead].data = buf;
    _preBuffer[_preBufferHead].len = len;

    _preBufferHead = (_preBufferHead + 1) % _preBufferSize;
    if (_preBufferCount < _preBufferSize) {
        _preBufferCount++;
    }
}

bool AVIRecorder::flushPreBuffer() {
    if (!_preBufferActive || _preBufferCount == 0) return true;
    if (!_recording || !_file) return false;

    // Calcular inicio del buffer circular
    uint8_t start = (_preBufferHead + _preBufferSize - _preBufferCount) % _preBufferSize;

    for (uint8_t i = 0; i < _preBufferCount; i++) {
        uint8_t idx = (start + i) % _preBufferSize;
        if (_preBuffer[idx].data && _preBuffer[idx].len > 0) {
            writeFrameChunk(_preBuffer[idx].data, _preBuffer[idx].len);
            _frameCount++;
        }
    }

    // Liberar memoria
    for (uint8_t i = 0; i < _preBufferSize; i++) {
        freePreBufferSlot(i);
    }
    _preBufferCount = 0;
    _preBufferHead = 0;

    return true;
}

// ===== ROTACION =====

void AVIRecorder::checkRotation() {
    if (!_recording) return;

    if (millis() - _startTime > CLIP_DURATION_MS) {
        Serial.println("[AVI] Rotacion de clip");
        stopRecording();

        String newFile = generateFilename();
        startRecording(newFile.c_str());
    }
}

String AVIRecorder::generateFilename() {
    char file[64];
    snprintf(file, sizeof(file), "/DCIM/CLIP_%04d.avi", _fileCounter++);
    return String(file);
}

void AVIRecorder::markImportant() {
    _important = true;
}

size_t AVIRecorder::getCurrentFileSize() const {
    if (_recording && _file) {
        return _file.size();
    }
    return 0;
}

// ===== INTERNOS AVI =====

bool AVIRecorder::writeAVIHeader(uint16_t width, uint16_t height) {
    // RIFF header
    writeFourCC(_file, "RIFF");
    _offsetFileSize = _file.position();
    writeUint32(_file, 0);
    writeFourCC(_file, "AVI ");

    // LIST hdrl
    writeFourCC(_file, "LIST");
    writeUint32(_file, 4 + 56 + 4 + 48 + 40);
    writeFourCC(_file, "hdrl");

    // avih (56 bytes)
    writeFourCC(_file, "avih");
    writeUint32(_file, 56);
    writeUint32(_file, 100000);
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 0x110);
    _offsetTotalFrames = _file.position();
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 1);
    writeUint32(_file, 0);
    writeUint32(_file, width);
    writeUint32(_file, height);
    for (int i = 0; i < 4; i++) writeUint32(_file, 0);

    // LIST strl
    writeFourCC(_file, "LIST");
    writeUint32(_file, 4 + 48 + 40);
    writeFourCC(_file, "strl");

    // strh (48 bytes)
    writeFourCC(_file, "strh");
    writeUint32(_file, 48);
    writeFourCC(_file, "vids");
    writeFourCC(_file, "MJPG");
    writeUint32(_file, 0);
    writeUint16(_file, 0);
    writeUint16(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 1);
    writeUint32(_file, 10);
    writeUint32(_file, 0);
    _offsetLength = _file.position();
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint16(_file, 0);
    writeUint16(_file, 0);
    writeUint16(_file, width);
    writeUint16(_file, height);

    // strf BITMAPINFOHEADER (40 bytes)
    writeFourCC(_file, "strf");
    writeUint32(_file, 40);
    writeUint32(_file, 40);
    writeUint32(_file, width);
    writeUint32(_file, height);
    writeUint16(_file, 1);
    writeUint16(_file, 24);
    writeFourCC(_file, "MJPG");
    writeUint32(_file, width * height * 3);
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 0);
    writeUint32(_file, 0);

    // LIST movi
    writeFourCC(_file, "LIST");
    _offsetMoviSize = _file.position();
    writeUint32(_file, 0);
    writeFourCC(_file, "movi");

    return true;
}

bool AVIRecorder::writeFrameChunk(const uint8_t* data, size_t len) {
    if (!_file) return false;

    writeFourCC(_file, "00dc");
    writeUint32(_file, len);
    _file.write(data, len);

    // Padding a 2 bytes
    if (len % 2 != 0) {
        _file.write((uint8_t)0);
    }

    _moviSize += 8 + len + (len % 2);
    return true;
}

bool AVIRecorder::finalizeAVI() {
    if (!_file) return false;

    size_t fileSize = _file.size();

    // Actualizar fileSize (offset 4)
    _file.seek(_offsetFileSize);
    writeUint32(_file, fileSize - 8);

    // Actualizar totalFrames en avih
    _file.seek(_offsetTotalFrames);
    writeUint32(_file, _frameCount);

    // Actualizar length en strh
    _file.seek(_offsetLength);
    writeUint32(_file, _frameCount);

    // Actualizar moviSize
    _file.seek(_offsetMoviSize);
    writeUint32(_file, _moviSize + 4); // +4 por "movi"

    _file.seek(fileSize);

    return true;
}

void AVIRecorder::freePreBufferSlot(uint8_t idx) {
    if (_preBuffer && idx < _preBufferSize) {
        if (_preBuffer[idx].data) {
            free(_preBuffer[idx].data);
            _preBuffer[idx].data = nullptr;
            _preBuffer[idx].len = 0;
        }
    }
}

void AVIRecorder::writeUint16(File& f, uint16_t val) {
    f.write((uint8_t)(val & 0xFF));
    f.write((uint8_t)((val >> 8) & 0xFF));
}

void AVIRecorder::writeUint32(File& f, uint32_t val) {
    f.write((uint8_t)(val & 0xFF));
    f.write((uint8_t)((val >> 8) & 0xFF));
    f.write((uint8_t)((val >> 16) & 0xFF));
    f.write((uint8_t)((val >> 24) & 0xFF));
}

void AVIRecorder::writeFourCC(File& f, const char* fourcc) {
    f.write((const uint8_t*)fourcc, 4);
}
