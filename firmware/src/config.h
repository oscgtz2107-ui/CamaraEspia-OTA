#ifndef CONFIG_H
#define CONFIG_H

// ===== VERSION =====
#define FIRMWARE_VERSION "1.0.0"

// ===== PINES =====
#define PIN_BOOT          0
#define PIN_PIR           1   // INPUT - PLUG-AND-PLAY (opcional)
#define PIN_SERVO         2
#define PIN_BATTERY       3   // ADC1_CH2 - PLUG-AND-PLAY (opcional)
#define PIN_CAM_SIOD      4
#define PIN_CAM_SIOC      5
#define PIN_CAM_VSYNC     6
#define PIN_CAM_HREF      7
#define PIN_CAM_D2        8
#define PIN_CAM_D1        9
#define PIN_CAM_D3       10
#define PIN_CAM_D0       11
#define PIN_CAM_D4       12
#define PIN_CAM_PCLK     13
#define PIN_CAM_XCLK     15
#define PIN_CAM_D7       16
#define PIN_CAM_D6       17
#define PIN_CAM_D5       18
#define PIN_SD_CS        21   // PLUG-AND-PLAY (opcional)
#define PIN_SD_MOSI      40   // PLUG-AND-PLAY (opcional)
#define PIN_SD_CLK       41   // PLUG-AND-PLAY (opcional)
#define PIN_SD_MISO      42   // PLUG-AND-PLAY (opcional)
#define PIN_LED          47

// ===== SERVO (LEDC) =====
#define SERVO_CHANNEL     2
#define SERVO_FREQ_HZ     50
#define SERVO_RESOLUTION  16  // bits
#define SERVO_MIN_US      500   // pulso minimo en microsegundos
#define SERVO_MAX_US      2500  // pulso maximo en microsegundos
#define SERVO_DEFAULT_ANGLE 90

// ===== CAMARA =====
#define CAM_MODEL_OV3660
#define XCLK_FREQ_HZ     20000000  // 20MHz para OV3660

// ===== RESOLUCIONES =====
enum Resolution {
    RES_VGA,    // 640x480, ~10fps
    RES_HD,     // 1280x720, ~5fps
    RES_3MP     // 2048x1536, ~1fps
};

// ===== MODO GRABACION =====
enum RecordMode {
    MODE_SOLO_MOVIMIENTO,   // graba solo cuando PIR detecta
    MODE_GRABAR_TODO        // grabacion continua en clips de 5 min
};

// ===== WIFI AP =====
#define WIFI_AP_SSID      "CamaraEspia"
#define WIFI_AP_PASS      ""  // Sin contraseña para debug. Restaurar antes de producción
#define WIFI_AP_CHANNEL   1   // default canal 1, auto-select en runtime si es necesario
#define WIFI_AP_IP        "192.168.4.1"
#define WIFI_AP_GATEWAY   "192.168.4.1"
#define WIFI_AP_SUBNET    "255.255.255.0"
#define WIFI_AP_MAX_CONN  4

// ===== TELEGRAM =====
#define TG_BOT_TOKEN            "8944156353:AAEAeK3J1y8h8b0rotAJPqB6IlSiOvhdaKc"
#define TG_COOLDOWN_MS          30000   // 30s entre fotos enviadas a Telegram
#define TG_SEND_ON_MOTION       true    // enviar foto al detectar movimiento
#define TG_MAX_USERS            8       // max usuarios registrados
#define TG_DEFAULT_LANG         0       // LANG_ES
#define TG_POLL_ACTIVE_MS       2000    // polling cada 2s cuando activo
#define TG_POLL_IDLE_MS         15000   // polling cada 15s cuando idle

// ===== TIEMPOS =====
#define SLEEP_TIMEOUT_MS      30000   // 30s idle -> sleep
#define STREAM_TIMEOUT_MS     60000   // 60s sin clientes -> stop stream
#define CLIP_DURATION_MS      300000  // 5 minutos por clip
#define PIR_COOLDOWN_MS       3000    // cooldown tras PIR
#define PRE_BUFFER_SECONDS    2       // segundos de pre-buffer
#define WDT_TIMEOUT_S         30      // watchdog 30 segundos
#define STATUS_INTERVAL_MS    2000    // envio status cada 2s
#define BATTERY_INTERVAL_MS   30000   // lectura bateria cada 30s
#define WS_PING_INTERVAL_MS   5000    // ping WS cada 5s
#define WS_RECONNECT_BASE_MS  3000    // reconexion base
#define WS_RECONNECT_MAX_MS   30000   // reconexion maxima
#define SERVO_DEBOUNCE_MS     200     // debounce slider

// ===== SD =====
#define SD_MIN_FREE_MB        500     // borrar antiguos si < 500MB
#define SD_MOUNT_POINT        "/sd"

// ===== BATERIA =====
#define BATT_VOLT_MIN         3.0f    // 0%
#define BATT_VOLT_MAX         4.2f    // 100%
#define BATT_ADC_MAX          2.1f    // divisor 100k+100k: 4.2V -> 2.1V
#define BATT_ADC_ATTEN        ADC_11db // rango 0-3.3V con atenuacion

// ===== CALIDAD JPEG =====
#define JPEG_QUALITY_DEFAULT  10      // 1-63, menor = mejor calidad
#define JPEG_QUALITY_MIN      1
#define JPEG_QUALITY_MAX      63

// ===== WATCHDOG =====
// WDT safe boot flag - se implementa como variable RTC_DATA_ATTR en main.cpp
// NO usar como define, RTC_DATA_ATTR es un atributo de variable
#define WDT_SAFE_BOOT_MAGIC   0xDEADBEEF

// ===== OTA =====
#define OTA_URL_MAX_SIZE      256

// ===== AUTH =====
#define JWT_SECRET            "c4m4r4_3sp14_s3cr3t_2024"
#define JWT_EXPIRY_HOURS      24
#define AUTH_USER_DEFAULT     "admin"
#define AUTH_PASS_DEFAULT     "admin123"

// ===== ARCHIVOS =====
#define FILENAME_FORMAT       "/%Y-%m-%d_%H-%M-%S.avi"
#define TEMP_EXT              ".tmp"
#define AVI_EXT               ".avi"
#define SPIFFS_MOUNT          "/spiffs"

// ===== DETECCION PLUG-AND-PLAY =====
#define HW_DETECTION_SAMPLES      5       // lecturas ADC para detectar bateria
#define HW_DETECTION_ADC_MIN      100     // si ADC < 100, asumir no conectado (0 = flotante)
#define HW_DETECTION_ADC_MAX      4000    // si ADC > 4000, asumir pin flotante/pullup
#define HW_PIR_DETECTION_MS       2000    // ms esperando cambio de estado para detectar PIR
#define HW_PIR_DETECTION_SAMPLES  20      // lecturas del pin durante detection

#endif // CONFIG_H
