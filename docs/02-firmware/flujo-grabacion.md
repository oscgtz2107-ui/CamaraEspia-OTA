# Flujo de Grabacion - CamaraEspia ESP32-S3

## Visión general

El sistema de grabacion gestiona dos modos de operacion: "Solo Movimiento" (grabacion selectiva por PIR) y "Grabar Todo" (grabacion continua por segmentos). Ambos modos utilizan un buffer circular en PSRAM para preservar los frames anteriores al evento de grabacion.

---

## Modo "Solo Movimiento"

Este modo graba clips unicamente cuando el sensor PIR detecta movimiento. Es el modo por defecto y el mas eficiente en almacenamiento y bateria.

### Paso 1: Pre-buffer en PSRAM

```
Estado: AWAKE_IDLE (camara en bajo consumo)
```

La camara opera a baja tasa (1 fps) capturando frames JPEG y almacenandolos en un buffer circular en PSRAM.

- **Tasa**: 1 frame por segundo
- **Duracion del pre-buffer**: 2 segundos
- **Buffer total**: 2 frames (a 640x480 JPEG ~20-40KB = ~40-80KB total en PSRAM)
- **Almacenamiento**: Ring buffer con puntero de escritura circular

```cpp
// Estructura del pre-buffer
struct PreBuffer {
    uint8_t* frames[PRE_BUFFER_SIZE];  // 2 frames
    uint16_t frame_sizes[PRE_BUFFER_SIZE];
    uint8_t write_index;
    uint8_t count;
};

// Captura de pre-buffer (cada 1 segundo)
void capture_pre_buffer() {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
        pre_buffer.frames[pre_buffer.write_index] = malloc(fb->len);
        memcpy(pre_buffer.frames[pre_buffer.write_index], fb->buf, fb->len);
        pre_buffer.frame_sizes[pre_buffer.write_index] = fb->len;
        pre_buffer.write_index = (pre_buffer.write_index + 1) % PRE_BUFFER_SIZE;
        pre_buffer.count = min(pre_buffer.count + 1, PRE_BUFFER_SIZE);
        esp_camera_fb_return(fb);
    }
}
```

### Paso 2: Deteccion PIR (ISR)

```
GPIO1 → Flanco HIGH → ISR
```

Cuando el sensor PIR detecta movimiento, genera una interrupcion en GPIO1.

```cpp
// ISR del PIR
void IRAM_ATTR pir_isr_handler(void* arg) {
    pir_flag = true;
    // Si esta en light sleep, esto despierta el CPU
}
```

### Paso 3: Wake desde Light Sleep

```
GPIO1 HIGH → esp_light_sleep_start() retorna → ~10ms
```

Si el sistema estaba dormido:

1. La ISR setea `pir_flag = true`.
2. `esp_light_sleep_start()` retorna inmediatamente.
3. El CPU se reactiva en ~10ms.
4. Se verifica la fuente de wake: `esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO`.

```cpp
// Despues del wake
esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
if (cause == ESP_SLEEP_WAKEUP_GPIO) {
    // PIR o BOOT despertaron el sistema
    pir_flag = true;
}
```

### Paso 4: Restauracion y re-configuracion

```
Wake → NVS restore → Camera init → WiFi AP → Servo position → ~50ms
```

Desde NVS se recuperan:

- Angulo del servo
- Modo de captura
- Resolucion y calidad
- Ultimo estado conocido

Se re-inicializan los perifericos en este orden:

1. SPI/SD (montar tarjeta)
2. Camara (`esp_camera_init()`)
3. WiFi AP (si hay clientes pendientes)
4. Servo (reposicionar con movimiento suave)
5. LED indicador

**Tiempo total de restauracion**: ~50ms desde wake.

### Paso 5: Flush del pre-buffer

```
Pre-buffer (2s) → archivo .tmp en SD
```

Los frames del pre-buffer se escriben al archivo AVI antes de continuar con la grabacion en vivo:

```cpp
// Abrir archivo temporal
FILE* f = fopen("/sdcard/recording.tmp", "wb");
write_avi_header(f, resolution, fps);

// Vaciar pre-buffer al archivo
for (int i = 0; i < pre_buffer.count; i++) {
    write_frame_to_avi(f, pre_buffer.frames[i], 
                       pre_buffer.frame_sizes[i]);
}
```

**Esto garantiza que los 2 segundos ANTES del evento PIR estan incluidos en el clip.**

### Paso 6: Grabacion continua

```
 frames en vivo → .tmp (maximo 30 segundos)
```

Despues del flush del pre-buffer, la camara captura frames continuamente y los escribe al archivo:

- **Resolucion**: Configurable (VGA, HD, MP3)
- **Tasa**: ~5-15 fps segun resolucion
- **Calidad JPEG**: Configurable (1-63, menor = mejor calidad)
- **Maximo**: 30 segundos por clip, o hasta que PIR se ponga LOW + cooldown de 3 segundos

### Paso 7: Cierre del clip

```
PIR LOW por 3 segundos → cerrar AVI → renombrar .tmp → .avi
```

Cuando el sensor PIR detecta que no hay movimiento:

1. Se inicia un timer de cooldown de 3 segundos.
2. Si PIR vuelve a HIGH durante el cooldown, se reinicia el timer.
3. Cuando el cooldown expira, se cierra el archivo AVI:

```cpp
// Cierre del clip
write_avi_footer(f, frame_count, total_duration);
fclose(f);

// Renombrar de temporal a final
rename("/sdcard/recording.tmp", 
       "/sdcard/2024-01-15_14-30-00.avi");
```

### Paso 8: Gestion de espacio

```
SD < 500MB libres → borrar clips antiguos no importantes
```

Si la tarjeta SD se queda sin espacio:

1. Se ordenan los clips por fecha (mas antiguo primero).
2. Se saltan los clips marcados como "IMPORTANTE".
3. Se eliminan clips hasta liberar al menos 500MB.

```cpp
// Gestion de espacio
uint32_t free_mb = get_sd_free_mb();
while (free_mb < 500) {
    char* oldest = get_oldest_non_important_clip();
    if (oldest) {
        delete_clip(oldest);
        free_mb = get_sd_free_mb();
    } else {
        break;  // Todos los clips son importantes
    }
}
```

### Paso 9: Cooldown y retorno

```
Clip cerrado → esperar 10s → volver a pre-buffer → posiblemente SLEEP
```

Despues de cerrar el clip:

1. Cooldown de 10 segundos para evitar grabacion inmediata por rebotes del PIR.
2. Reanudar pre-buffer en PSRAM.
3. Si no hay actividad en 30 segundos → entrar en Light Sleep.

---

## Modo "Grabar Todo"

Este modo graba continuamente sin depender del sensor PIR. Ideal para situaciones donde se quiere cobertura completa.

### Paso 1: Inicio de grabacion continua

```
AWAKE_IDLE + modo "Grabar Todo" → iniciar clip
```

Al entrar en este modo:

1. Se crea inmediatamente un archivo `.tmp` en SD.
2. Se escribe el header AVI.
3. Los frames se escriben continuamente.

### Paso 2: Rotacion por tiempo

```
Cada 5 minutos → cerrar clip → crear nuevo clip
```

El sistema graba en segmentos de 5 minutos:

- **Duracion maxima por clip**: 5 minutos (300 segundos)
- Al completar 5 minutos:
  1. Se cierra el header AVI con el frame count y duracion reales.
  2. Se renombra `.tmp` a `.avi` con timestamp.
  3. Se crea un nuevo `.tmp` para el siguiente clip.
  4. Se verifica espacio en SD.

### Paso 3: Rotacion por espacio

```
SD < 500MB → borrar clips mas antiguos no importantes
```

Igual que en "Solo Movimiento", pero con mayor frecuencia de escritura:

- Verificacion de espacio cada 60 segundos.
- Prioridad de borrado: clips no importantes mas antiguos primero.
- Nunca borrar clips marcados como "IMPORTANTE".

### Paso 4: Streaming simultaneo

```
WiFi ON + grabando → duplicar frames → SD + WiFi
```

Si hay un cliente conectado al stream mientras se graba:

1. Cada frame capturado se duplica:
   - Copia 1: Escrita al archivo .tmp en SD
   - Copia 2: Enviada via multipart stream al cliente WiFi
2. La grabacion a SD tiene prioridad sobre el envio WiFi.
3. Si la SD esta lenta, se reduce la calidad del stream (mas compresion JPEG) para mantener la grabacion.
4. Si WiFi se degrada, la grabacion continua sin interrupcion.

---

## Formato AVI

Los archivos grabados utilizan el formato AVI (Audio Video Interleave) con frames JPEG individuales.

### Estructura del archivo

```
┌─────────────────────────────────────────┐
│           RIFF Header                    │
│  "RIFF" + size + "AVI "                 │
├─────────────────────────────────────────┤
│           Header List (hdrl)             │
│  ┌─────────────────────────────────┐    │
│  │  Main AVI Header (avih)         │    │
│  │  - Microseconds per frame       │    │
│  │  - Width, Height                │    │
│  │  - Frame count                  │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │  Stream Header (strl)           │    │
│  │  - Codec: MJPG                  │    │
│  │  - Width, Height                │    │
│  │  - Frame rate                   │    │
│  └─────────────────────────────────┘    │
├─────────────────────────────────────────┤
│           Movie Data (movi)              │
│  ┌─────────────────────────────────┐    │
│  │  Frame 0 (LIST)                 │    │
│  │  "00dc" + size + JPEG bytes     │    │
│  ├─────────────────────────────────┤    │
│  │  Frame 1 (LIST)                 │    │
│  │  "00dc" + size + JPEG bytes     │    │
│  ├─────────────────────────────────┤    │
│  │  ...                            │    │
│  ├─────────────────────────────────┤    │
│  │  Frame N (LIST)                 │    │
│  │  "00dc" + size + JPEG bytes     │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

### Cada frame como chunk LIST

```cpp
// Estructura de cada frame en el AVI
struct AviFrame {
    uint32_t fourcc;      // "LIST" o "00dc"
    uint32_t size;        // Tamano del frame JPEG
    uint32_t timestamp;   // Timestamp en microsegundos
    uint8_t* jpeg_data;   // Bytes JPEG del frame
};
```

### Header AVI (simplificado)

```cpp
// Main AVI Header
struct AviMainHeader {
    uint32_t microsec_per_frame;  // 1000000 / fps
    uint32_t max_bytes_per_sec;   // Tasa maxima
    uint32_t padding;             // 0
    uint32_t flags;               // 0x10 (has index)
    uint32_t total_frames;        // Numero total de frames
    uint32_t initial_frames;      // 0
    uint32_t streams;             // 1 (solo video)
    uint32_t buffer_size;         // 0
    uint32_t width;               // Ancho en pixels
    uint32_t height;              // Alto en pixels
    uint32_t reserved[4];         // 0
};
```

---

## Double-buffer (Seguridad de archivos)

Todos los archivos de grabacion se escriben primero como `.tmp` y solo se renombran a `.avi` despues de un cierre exitoso.

### Proteccion contra fallos

```
Si crash o perdida de energia durante grabacion:
  - El archivo .tmp queda incompleto
  - Al reiniciar, se detecta .tmp sin cerrar
  - Opciones:
    a) Eliminar .tmp (descartar clip incompleto)
    b) Recuperar frames validos del .tmp
    c) Marcar como "CORRUPTO" para revision manual
```

### Implementacion

```cpp
// Al inicio de grabacion
void start_recording() {
    current_file = fopen("/sdcard/recording.tmp", "wb");
    write_avi_header(current_file);
    is_recording = true;
}

// Al finalizar grabacion
void stop_recording() {
    write_avi_footer(current_file, frame_count, duration);
    fclose(current_file);
    
    // Renombrar solo despues de cierre exitoso
    rename("/sdcard/recording.tmp", final_filename);
    is_recording = false;
}

// Al iniciar (detectar archivos temporales)
void check_temp_files() {
    if (file_exists("/sdcard/recording.tmp")) {
        // Intentar recuperar o eliminar
        recover_or_delete_temp();
    }
}
```

### Recuperacion de archivos temporales

Si el sistema detecta un `.tmp` al iniciar:

1. Leer el header AVI del `.tmp`.
2. Verificar si el header es valido (magic bytes correctos).
3. Contar frames validos en el archivo.
4. Si hay suficientes frames (>10): intentar cerrar el AVI correctamente.
5. Si el archivo esta muy corrupto: eliminarlo.

---

## Gestion de espacio en SD

### Estrategia de borrado

```
Umbral critico: 500MB libres
Umbral de warning: 1GB libres

Al alcanzar umbral critico:
  1. Listar todos los clips ordenados por fecha
  2. Filtrar: excluir clips con flag "IMPORTANTE"
  3. Eliminar clips mas antiguos primero
  4. Liberar al menos 500MB
  5. Si no hay suficientes clips no importantes:
     - No eliminar nada
     - Enviar notificacion al usuario
     - La grabacion continuara con espacio limitado
```

### Clips importantes

Los usuarios pueden marcar clips como "IMPORTANTE" via la API:

```cpp
// Marcar clip como importante
void mark_important(const char* filename, bool important) {
    // Actualizar metadata en NVS o archivo de indice
    update_clip_metadata(filename, "important", important);
}

// Verificar si un clip es importante
bool is_important(const char* filename) {
    return get_clip_metadata(filename, "important");
}
```

### Indices de clips

Los clips se indexan en un archivo JSON en la SD:

```json
{
  "clips": [
    {
      "name": "2024-01-15_14-30-00.avi",
      "size": 12500000,
      "date": "2024-01-15T14:30:00",
      "duration": 30,
      "resolution": "vga",
      "important": false
    }
  ]
}
```

---

## Timing de grabacion

### "Solo Movimiento" - Timeline completo

```
t=0.0s    Pre-buffer activo (1fps, 2 frames en PSRAM)
          ↓
t=5.0s    PIR detecta movimiento (GPIO1 HIGH)
          ↓
t=5.01s   Wake desde light sleep (~10ms)
          ↓
t=5.06s   Restauracion completa (~50ms)
          ↓
t=5.06s   Flush pre-buffer (2 frames) a .tmp
          ↓
t=5.1s    Grabacion en vivo inicia
          ↓
t=7.0s    PIR sigue activo, grabando
          ↓
t=10.0s   PIR LOW (movimiento termina)
          ↓
t=10.0s   Cooldown de 3 segundos inicia
          ↓
t=13.0s   Cooldown completo, cerrar clip
          ↓
t=13.1s   Renombrar .tmp → .avi
          ↓
t=13.1s   Cooldown post-grabacion (10s)
          ↓
t=23.1s   Volver a pre-buffer
          ↓
t=53.1s   Sin actividad en 30s → Light Sleep
```

### "Grabar Todo" - Timeline de rotacion

```
t=0.0s    Iniciar clip 1 (.tmp)
          ↓
t=300.0s  5 minutos completados, cerrar clip 1
          ↓
t=300.1s  Renombrar clip 1 (.tmp → .avi)
          ↓
t=300.1s  Iniciar clip 2 (.tmp)
          ↓
t=600.0s  5 minutos completados, cerrar clip 2
          ↓
t=600.1s  Renombrar clip 2 (.tmp → .avi)
          ↓
t=600.1s  Verificar espacio en SD
          ↓
          ... (continua indefinidamente)
```

---

## Consideraciones criticas

1. **Buffer circular en PSRAM**: El pre-buffer utiliza PSRAM (~8MB disponibles en ESP32-S3 N16R8). Con frames VGA (~20-40KB), caben ~200-400 frames. Para 2 segundos a 1fps, solo se necesitan 2 frames.

2. **Escritura SD**: La velocidad de escritura de la SD es critica. Usar SPI a 40MHz minimo. Si la SD es lenta (<2MB/s), se reducira el framerate de grabacion.

3. **Integridad del AVI**: El header AVI se escribe al inicio pero se actualiza al final (frame count total). Si el archivo se cierra abruptamente, el header podria estar incompleto.

4. **Coordinacion grabar/grabar+streamar**: Cuando ambos estan activos, cada frame se captura una sola vez y se duplica en memoria. Esto usa ~2x RAM pero evita capturar frames duplicados.

5. **Power loss**: El double-buffer (.tmp) protege contra perdida de energia. El archivo final (.avi) nunca se toca hasta que el cierre es exitoso.
