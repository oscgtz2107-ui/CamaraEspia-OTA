#include "camera_manager.h"

CameraManager::CameraManager()
    : _initialized(false), _standby(false), _currentResolution(RES_VGA),
      _frameSize(FRAMESIZE_VGA) {
}

bool CameraManager::begin() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_pwdn = -1;
    config.pin_reset = -1;
    config.pin_xclk = PIN_CAM_XCLK;
    config.pin_sccb_sda = PIN_CAM_SIOD;
    config.pin_sccb_scl = PIN_CAM_SIOC;
    config.pin_d7 = PIN_CAM_D7;
    config.pin_d6 = PIN_CAM_D6;
    config.pin_d5 = PIN_CAM_D5;
    config.pin_d4 = PIN_CAM_D4;
    config.pin_d3 = PIN_CAM_D3;
    config.pin_d2 = PIN_CAM_D2;
    config.pin_d1 = PIN_CAM_D1;
    config.pin_d0 = PIN_CAM_D0;
    config.pin_vsync = PIN_CAM_VSYNC;
    config.pin_href = PIN_CAM_HREF;
    config.pin_pclk = PIN_CAM_PCLK;
    config.xclk_freq_hz = XCLK_FREQ_HZ;
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_VGA;
    config.jpeg_quality = JPEG_QUALITY_DEFAULT;
    config.fb_count = 2;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;

    // Guardar config SIN inicializar el hardware (sin DMA, sin I2S).
    // Esto evita VSYNC overflow → TG1WDT durante el boot.
    // reinit() usara esta config cuando alguien necesite la camara.
    _config = config;
    _currentResolution = RES_VGA;
    _frameSize = FRAMESIZE_VGA;
    _initialized = false;

    Serial.printf("[CAM] Config guardada (lazy) - I2S NO arrancado\n");
    return true;
}

uint8_t* CameraManager::captureJPEG(size_t* outLen) {
    if (!_initialized) {
        *outLen = 0;
        return nullptr;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[CAM] FALLO al capturar frame");
        *outLen = 0;
        return nullptr;
    }

    uint8_t* buffer = (uint8_t*)ps_malloc(fb->len);
    if (!buffer) {
        buffer = (uint8_t*)malloc(fb->len);
    }

    if (!buffer) {
        Serial.printf("[CAM] Sin memoria para frame (%d bytes)\n", (int)fb->len);
        esp_camera_fb_return(fb);
        *outLen = 0;
        return nullptr;
    }

    memcpy(buffer, fb->buf, fb->len);
    *outLen = fb->len;
    esp_camera_fb_return(fb);
    return buffer;
}

bool CameraManager::captureJPEGToBuffer(uint8_t* buffer, size_t maxLen, size_t* outLen) {
    if (!_initialized) {
        *outLen = 0;
        return false;
    }

    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("[CAM] FALLO al capturar frame");
        *outLen = 0;
        return false;
    }

    if (fb->len > maxLen) {
        Serial.printf("[CAM] Frame (%d bytes) excede buffer (%d bytes)\n",
                      (int)fb->len, (int)maxLen);
        esp_camera_fb_return(fb);
        *outLen = 0;
        return false;
    }

    memcpy(buffer, fb->buf, fb->len);
    *outLen = fb->len;
    esp_camera_fb_return(fb);
    return true;
}

bool CameraManager::setResolution(Resolution res) {
    if (!_initialized) return false;

    sensor_t* s = esp_camera_sensor_get();
    if (!s) return false;

    framesize_t fs = resolutionToFrameSize(res);
    if (s->set_framesize(s, fs) != 0) {
        // Fallback a VGA si la resolucion no es soportada
        Serial.printf("[CAM] Resolucion %d no soportada, usando VGA\n", (int)res);
        s->set_framesize(s, FRAMESIZE_VGA);
        _frameSize = FRAMESIZE_VGA;
        _currentResolution = RES_VGA;
        return false;
    }

    _frameSize = fs;
    _currentResolution = res;

    const char* resName[] = {"VGA", "HD", "3MP"};
    Serial.printf("[CAM] Resolucion cambiada a %s\n", resName[(int)res]);
    return true;
}

Resolution CameraManager::getResolution() const {
    return _currentResolution;
}

bool CameraManager::standby() {
    if (!_initialized) return false;

    sensor_t* s = esp_camera_sensor_get();
    if (!s) return false;

    s->set_reg(s, 0x3008, 0x01, 0x01);
    Serial.println("[CAM] Standby");
    _standby = true;
    return true;
}

bool CameraManager::wake() {
    if (!_initialized) return false;

    sensor_t* s = esp_camera_sensor_get();
    if (!s) return false;

    s->set_reg(s, 0x3008, 0x01, 0x00);
    Serial.println("[CAM] Wake");
    _standby = false;
    return true;
}

bool CameraManager::isInitialized() const {
    return _initialized;
}

bool CameraManager::isStandby() const {
    return _standby;
}

size_t CameraManager::getEstimatedFrameSize() const {
    switch (_currentResolution) {
        case RES_VGA: return 50 * 1024;
        case RES_HD:  return 120 * 1024;
        case RES_3MP: return 250 * 1024;
        default:      return 50 * 1024;
    }
}

framesize_t CameraManager::resolutionToFrameSize(Resolution res) const {
    switch (res) {
        case RES_VGA: return FRAMESIZE_VGA;
        case RES_HD:  return FRAMESIZE_HD;
        case RES_3MP: return FRAMESIZE_UXGA;
        default:      return FRAMESIZE_VGA;
    }
}

bool CameraManager::configureSensor() {
    sensor_t* s = esp_camera_sensor_get();
    if (!s) return false;

    s->set_brightness(s, 0);
    s->set_contrast(s, 0);
    s->set_saturation(s, 0);
    s->set_special_effect(s, 0);
    s->set_whitebal(s, 1);
    s->set_awb_gain(s, 1);
    s->set_wb_mode(s, 0);
    s->set_exposure_ctrl(s, 1);
    s->set_aec2(s, 1);
    s->set_gain_ctrl(s, 1);
    s->set_agc_gain(s, 0);
    s->set_gainceiling(s, (gainceiling_t)6);
    s->set_bpc(s, 0);
    s->set_wpc(s, 1);
    s->set_raw_gma(s, 1);
    s->set_lenc(s, 1);
    s->set_hmirror(s, 0);
    s->set_vflip(s, 0);
    s->set_dcw(s, 1);
    s->set_colorbar(s, 0);

    return true;
}

bool CameraManager::deinit() {
    if (!_initialized) return true;

    esp_err_t err = esp_camera_deinit();
    _initialized = false;

    if (err == ESP_OK) {
        Serial.println("[CAM] Deinicializada (I2S DMA + TG detenidos)");
    } else {
        Serial.printf("[CAM] Error_deinit: %s\n", esp_err_to_name(err));
    }
    return (err == ESP_OK);
}

bool CameraManager::reinit() {
    if (_initialized) {
        deinit();
    }

    esp_err_t err = esp_camera_init(&_config);
    if (err != ESP_OK) {
        Serial.printf("[CAM] FALLO reinit: %s\n", esp_err_to_name(err));
        _initialized = false;
        return false;
    }

    // Poner sensor en standby INMEDIATAMENTE despues de esp_camera_init().
    // El DMA I2S arranca y el sensor genera VSYNC → overflow → TG1WDT si
    // nadie lee frames. Standby detiene el sensor antes de overflow.
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_reg(s, 0x3008, 0x01, 0x01);  // Standby mode
        _standby = true;
    }

    configureSensor();
    _initialized = true;
    Serial.println("[CAM] Reinicializada OK (standby)");
    return true;
}
