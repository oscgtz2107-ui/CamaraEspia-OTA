# CamaraEspia-ESP32S3

Camara de seguridad/espia compacta basada en ESP32-S3 con interfaz PWA.

## Caracteristicas

- Stream MJPEG en vivo via WiFi AP
- Control remoto de servo 0-180 grados
- Grabacion a MicroSD: "Solo Movimiento" (PIR) o "Grabar Todo"
- Buffer circular de pre-captura 2s en PSRAM
- Modo sleep inteligente (Light Sleep)
- OTA updates via HTTP
- PWA instalable (offline-first)
- Bateria 18650 con monitor de voltaje

## Lista de Materiales (BOM)

| # | Componente | Cantidad | Notas |
|---|-----------|----------|-------|
| 1 | ESP32-S3 N16R8 (16MB Flash, 8MB PSRAM) | 1 | Con camara OV3660 integrada |
| 2 | MicroSD Card (8-32GB, Class 10) | 1 | FAT32 |
| 3 | Sensor PIR HC-SR501 | 1 | Deteccion de movimiento |
| 4 | Servo SG90 o MG90S | 1 | 180 grados, engranajes plasticos/metal |
| 5 | Modulo J5019 | 1 | Carga LiPo + boost 5V |
| 6 | Bateria 18650 2500mAh | 1 | LiPo recargable |
| 7 | Resistencia 100kOhm | 2 | Divisor de voltaje bateria |
| 8 | Resistencia 220Ohm | 1 | LED indicador |
| 9 | LED 3mm | 1 | Estado (opcional) |
| 10 | Boton tactil | 1 | Wake-up (GPIO0/BOOT) |
| 11 | Protoboard | 1 | Para conexiones |
| 12 | Cable USB-C | 1 | Para programacion y carga |

## Compilacion

### Requisitos

- [PlatformIO](https://platformio.org/) instalado en VS Code
- Drivers USB para ESP32-S3

### Compilar

```bash
cd firmware
pio run
```

### Flashear

```bash
pio run -t upload
```

### Subir PWA a SPIFFS

```bash
# Primero copiar archivos de pwa/ a firmware/data/
# (usar tools/minify_pwa.ps1 o copiar manualmente)
pio run -t uploadfs
```

## Monitor Serial

```bash
pio device monitor
```

## Estructura del Repositorio

```
CamaraEspia-ESP32S3/
├── docs/           # Documentacion tecnica (hardware, firmware, PWA, plan)
├── firmware/       # Codigo fuente del firmware ESP32-S3
│   ├── src/        # Codigo fuente (main.cpp, managers, config)
│   ├── data/       # Archivos SPIFFS (PWA minificada)
│   └── partitions/ # Tabla de particiones flash
├── pwa/            # Codigo fuente de la PWA (desarrollo)
├── tools/          # Scripts de utilidad
└── README.md
```

## Licencia

MIT License
