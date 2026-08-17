#ifndef AVI_RECORDER_H
#define AVI_RECORDER_H

#include <Arduino.h>
#include "sd_manager.h"
#include "camera_manager.h"

struct PreCaptureFrame {
    uint8_t* data;
    size_t len;
};

class AVIRecorder {
public:
    AVIRecorder();

    bool begin(SDManager* sd, CameraManager* cam);

    // ===== GRABACION =====
    bool startRecording(const char* filename);
    bool stopRecording();
    bool writeFrame();
    bool isRecording() const;

    // ===== BUFFER CIRCULAR DE PRE-CAPTURA (PSRAM) =====
    bool startPreBuffer(uint8_t maxFrames = 20);
    void stopPreBuffer();
    void pushPreBuffer();
    bool flushPreBuffer();

    // ===== ROTACION DE CLIPS =====
    void checkRotation();
    static String generateFilename();
    void markImportant();
    size_t getCurrentFileSize() const;

private:
    SDManager* _sd;
    CameraManager* _camera;
    bool _recording;
    File _file;
    String _tempFile;
    String _finalFile;
    unsigned long _startTime;
    size_t _frameCount;
    size_t _moviSize;
    bool _important;

    // Offsets para finalize
    size_t _offsetFileSize;
    size_t _offsetTotalFrames;
    size_t _offsetLength;
    size_t _offsetMoviSize;

    // Buffer circular
    PreCaptureFrame* _preBuffer;
    uint8_t _preBufferSize;
    uint8_t _preBufferHead;
    uint8_t _preBufferCount;
    bool _preBufferActive;

    // Internos AVI
    bool writeAVIHeader(uint16_t width, uint16_t height);
    bool writeFrameChunk(const uint8_t* data, size_t len);
    bool finalizeAVI();
    void freePreBufferSlot(uint8_t idx);

    static void writeUint16(File& f, uint16_t val);
    static void writeUint32(File& f, uint32_t val);
    static void writeFourCC(File& f, const char* fourcc);

    static uint16_t _fileCounter;
};

#endif // AVI_RECORDER_H
