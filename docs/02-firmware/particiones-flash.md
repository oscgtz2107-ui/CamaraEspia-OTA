# Particiones Flash - CamaraEspia ESP32-S3

## Tabla de particiones

El ESP32-S3 N16R8 tiene 16MB de flash externo. La siguiente tabla define las particiones para el proyecto:

```
# Name,    Type,  SubType,  Offset,   Size,      Flags
nvs,        data,  nvs,      0x9000,   0x6000,
otadata,    data,  ota,      0xf000,   0x2000,
app0,       app,   ota_0,    0x10000,  0x200000,
app1,       app,   ota_1,    0x210000, 0x200000,
spiffs,     data,  spiffs,   0x410000, 0x180000,
coredump,   data,  coredump, 0x590000, 0x10000,
```

---

## Mapa de memoria

```
0x000000 +-------------------+
         | Bootloader        |  32KB (reservado por ROM)
0x008000 +-------------------+
         | (reservado)       |
0x009000 +-------------------+
         | NVS               |  24KB
0x00F000 +-------------------+
         | OTA Data          |  8KB
0x010000 +-------------------+
         |                   |
         |   app0 (OTA_0)    |  2MB
         |   Firmware slot 0 |
         |                   |
0x210000 +-------------------+
         |                   |
         |   app1 (OTA_1)    |  2MB
         |   Firmware slot 1 |
         |                   |
0x410000 +-------------------+
         |                   |
         |   SPIFFS          |  1.5MB
         |   PWA files       |
         |                   |
0x590000 +-------------------+
         |   Core Dump       |  64KB
0x5A0000 +-------------------+
         |                   |
         |   (reservado)     |  ~9.75MB
         |   hasta 16MB      |
         |                   |
0x1000000+-------------------+
```

---

## Descripcion de cada particion

### 1. NVS (Non-Volatile Storage)

| Propiedad | Valor |
|---|---|
| Tipo | data / nvs |
| Offset | 0x9000 |
| Tamano | 0x6000 (24KB) |
| Tipo de uso | Key-value store |

**Contenido almacenado**:

| Key | Tipo | Descripcion |
|---|---|---|
| wifi_ssid | string | Nombre de red WiFi (AP mode) |
| wifi_pass | string | Contrasena del AP WiFi |
| auth_user | string | Usuario para autenticacion HTTP |
| auth_pass | string | Contrasena cifrada para autenticacion |
| capture_mode | uint8 | 0=solo_movimiento, 1=grabar_todo |
| resolution | uint8 | 0=VGA, 1=HD, 2=MP3 |
| servo_angle | uint8 | Angulo actual del servo (0-180) |
| jpeg_quality | uint8 | Calidad JPEG (1-63) |
| wdt_flag | uint8 | Flag de reset por watchdog |
| last_boot_reason | uint8 | Causa del ultimo reinicio |
| ota_state | uint8 | Estado de la ultima actualizacion OTA |

**Por que 24KB**: NVS almacena configuracion y metadatos criticos. 24KB es suficiente para ~100-200 key-value pairs con margen para crecimiento futuro. NVS usa wear-leveling internamente, por lo que la vida util del flash no es preocupacion en esta particion.

### 2. OTA Data

| Propiedad | Valor |
|---|---|
| Tipo | data / ota |
| Offset | 0xF000 |
| Tamano | 0x2000 (8KB) |
| Tipo de uso | Control de OTA |

**Contenido**: Dos estructuras `esp_ota_img_states_t` que indican cual particion de app (app0 o app1) es la activa y cual es la alternativa.

**Funcion**:
- ESP-IDF utiliza esta particion para gestionar el swap entre particiones de firmware.
- Contiene el estado de la ultima actualizacion: cual particion esta activa, si hubo una actualizacion reciente, y si la particion alternativa es valida.
- Se actualiza automaticamente durante el proceso OTA.

### 3. app0 (OTA_0) - Firmware Slot 0

| Propiedad | Valor |
|---|---|
| Tipo | app / ota_0 |
| Offset | 0x10000 |
| Tamano | 0x200000 (2MB) |
| Tipo de uso | Firmware compilado |

**Contenido**: Binario de firmware compilado con todas las librerias del proyecto.

**Tamano justificado**: El firmware de CamaraEspia incluye:
- Libreria de camara ESP32-S3 (~200-300KB)
- WiFi stack (~150-200KB)
- Servidor HTTP/WebSocket (~100-150KB)
- Driver SPI para SD (~30-50KB)
- Manejo de AVI/RIFF (~20-30KB)
- JSON parsing (~30-50KB)
- OTA manager (~20-30KB)
- Codigo de la aplicacion (~200-300KB)
- Stack y vectores de interrupcion (~50-100KB)

**Total estimado**: ~1.2-1.5MB. Con 2MB de espacio hay margen del 30-60% para crecimiento futuro o inclusion de nuevas funcionalidades (BLE, MQTT, etc.).

### 4. app1 (OTA_1) - Firmware Slot 1

| Propiedad | Valor |
|---|---|
| Tipo | app / ota_1 |
| Offset | 0x210000 |
| Tamano | 0x200000 (2MB) |
| Tipo de uso | Firmware alternativo (OTA) |

**Contenido**: Binario de firmware alternativo para actualizaciones OTA.

**Funcion**:
- Durante una actualizacion OTA, el nuevo firmware se escribe a esta particion.
- Si la verificacion SHA256 es exitosa, se marca como boot activa.
- En el proximo reinicio, el bootloader arranca desde app1.
- Si app1 falla (bootloop, WDT), el sistema puede revertir a app0.

### 5. SPIFFS

| Propiedad | Valor |
|---|---|
| Tipo | data / spiffs |
| Offset | 0x410000 |
| Tamano | 0x180000 (1.5MB) |
| Tipo de uso | Sistema de archivos para archivos estaticos |

**Contenido**: Archivos de la interfaz web PWA (Progressive Web App).

**Estructura de archivos**:

```
/
  index.html          (~15-25KB minificado)
  app.js              (~30-50KB minificado)
  style.css           (~10-20KB minificado)
  manifest.json       (~1-2KB)
  sw.js               (~5-10KB, service worker)
  icons/
    icon-192.png      (~5-10KB)
    icon-512.png      (~10-20KB)
  fonts/
    (si aplica)       (~20-50KB)
```

**Tamano estimado total**: ~200-300KB minificado. Con 1.5MB hay amplio margen para incluir mas assets (videos embebidos, imagenes adicionales, etc.) sin preocupacion de espacio.

**Por que SPIFFS y no LittleFS**: SPIFFS es el sistema de archivos por defecto en ESP-IDF para particiones de datos. LittleFS es una alternativa mas moderna pero requiere configuracion adicional. Para archivos estaticos inmutables, SPIFFS es suficiente.

### 6. Core Dump

| Propiedad | Valor |
|---|---|
| Tipo | data / coredump |
| Offset | 0x590000 |
| Tamano | 0x10000 (64KB) |
| Tipo de uso | Dump de memoria en caso de panic |

**Contenido**: Snapshot de la memoria RAM del CPU en el momento de un panic o crash.

**Funcion**:
- Cuando el sistema detecta un panic (exception, WDT timeout, abort), escribe el estado completo de registros y stack a esta particion.
- Al reiniciar, si se detecta un core dump valido, se puede analizar con `espcoredump` o `addr2line` para diagnosticar el cause del crash.
- Util para desarrollo y debugging de firmware en campo.

**Tamano**: 64KB es suficiente para un dump basico (registros + stack trace). Un dump completo de toda la SRAM requeriria ~512KB, pero para diagnostico basico 64KB es adecuado.

---

## Analisis de espacio

### Uso total de particiones

| Particion | Tamano | Uso |
|---|---|---|
| NVS | 24KB | Configuracion y metadatos |
| OTA Data | 8KB | Control de actualizaciones |
| app0 | 2MB | Firmware slot 0 |
| app1 | 2MB | Firmware slot 1 |
| SPIFFS | 1.5MB | Archivos PWA |
| Core Dump | 64KB | Diagnostico de crashes |
| **Total usado** | **~5.6MB** | |
| **Flash total** | **16MB** | |
| **Restante** | **~10.4MB** | Reservado |

### Espacio restante

De los 16MB totales, se usan aproximadamente 5.6MB para particiones definidas. El espacio restante (~10.4MB) queda sin particionar y reservado para:

- **Futuras expansiones**: Si se necesita mas espacio para firmware, se puede redimensionar app0/app1.
- **Datos temporales**: Si se implementa almacenamiento en flash (ademas de SD).
- **Particiones adicionales**: Cualquier nueva particion (ej: OTA extensa, partition de datos encriptados).

**Nota importante**: El espacio restante NO se puede usar directamente sin definir una particion. Las aplicaciones solo pueden leer/escribir dentro de sus particiones asignadas.

---

## Proceso OTA detallado

### Flujo de actualizacion

```
┌─────────────────────────────────────────────────┐
│  1. Cliente envia firmware via POST /api/ota    │
│     - Header X-SHA256 con hash esperado         │
│     - Body: binario firmware completo           │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│  2. Verificacion SHA256                         │
│     - Calcular SHA256 del firmware recibido     │
│     - Comparar con X-SHA256 del header          │
│     - Si no coincide: rechazar (400)            │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│  3. Escritura a particion inactiva              │
│     - Determinar particion inactiva (app0/app1) │
│     - Leer OTA data para saber cual usar        │
│     - Escribir firmware chunk por chunk         │
│     - Verificar integridad tras escritura       │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│  4. Marcar como boot activa                     │
│     - Actualizar OTA data                       │
│     - Marcar particion nueva como "confirm"     │
│     - La particion anterior queda como backup   │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│  5. Reinicio automatico                         │
│     - Timer de 5 segundos                       │
│     - Guardar estado actual en NVS              │
│     - esp_restart()                             │
└──────────────────────┬──────────────────────────┘
                       │
                       ▼
┌─────────────────────────────────────────────────┐
│  6. Boot desde nueva particion                  │
│     - Bootloader lee OTA data                   │
│     - Arranca desde app1 (nueva)                │
│     - Firmware nuevo se ejecuta                  │
│     - Si falla: revertir a app0                 │
└─────────────────────────────────────────────────┘
```

### Proteccion contra fallos OTA

- **Verificacion SHA256**: Previene escritura de firmware corrupto.
- **Dual-partition**: La particion anterior nunca se sobreescribe. Si la nueva falla, se puede revertir.
- **Bootloop detection**: Si el firmware nuevo falla 3 veces consecutivas (WDT), ESP-IDF revierte automaticamente a la particion anterior.
- **OTA data validation**: Se verifica la integridad de la OTA data antes de cada boot.

---

## Configuracion en platformio.ini

```ini
[env:esp32s3]
platform = espressif32@6.3.2
board = esp32-s3-devkitc-1
framework = arduino

; Flash 16MB
board_build.flash_size = 16MB
board_build.partitions = custom_partitions.csv

; PSRAM 8MB
board_build.arduino.memory_type = qio_opi
board_build.partitions = custom_partitions.csv
```

### Contenido de custom_partitions.csv

```csv
# Name,    Type,  SubType,  Offset,   Size,      Flags
nvs,        data,  nvs,      0x9000,   0x6000,
otadata,    data,  ota,      0xf000,   0x2000,
app0,       app,   ota_0,    0x10000,  0x200000,
app1,       app,  ota_1,    0x210000, 0x200000,
spiffs,     data,  spiffs,   0x410000, 0x180000,
coredump,   data,  coredump, 0x590000, 0x10000,
```

---

## Consideraciones importantes

1. **No modificar particiones en runtime**: Las particiones se definen en el CSV y se compilan en el bootloader. No se pueden cambiar sin recompilar el firmware.

2. **NVS wear-leveling**: NVS usa wear-leveling automatico. No es necesario preocuparse por la vida util del flash al escribir configuracion frecuentemente.

3. **SPIFFS es read-heavy**: La particion SPIFFS contiene archivos estaticos que raramente cambian. Las operaciones de escritura son minimas (solo durante actualizaciones de la PWA).

4. **Core dump space**: 64KB es suficiente para diagnostico basico. Si se necesita un dump completo, se puede redimensionar a 128KB o mas (restando del espacio reservado).

5. **OTA sin interrupcion**: El proceso OTA es seguro. Si la alimentacion falla durante la escritura, el firmware anterior sigue siendo bootable. La unica perdida es el firmware nuevo no instalado.

6. **SD card no esta en flash**: Las grabaciones y clips se almacenan en la tarjeta SD (externa), no en flash. Esto es intencional: la SD tiene mucho mas espacio (~32GB+) y es removible.
