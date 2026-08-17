# Arquitectura de Sleep - CamaraEspia ESP32-S3

## Por que Light Sleep y no Deep Sleep

La camara espia utiliza **Light Sleep** como estrategia principal de ahorro energetico. Deep Sleep no es viable para este proyecto por las siguientes razones:

### Limitaciones del Deep Sleep

- **PWM del servo perdido**: En Deep Sleep se apaga el dominio digital completo. El controlador LEDC (que genera la senal PWM para el servo) se destruye. Al despertar, el servo pierde su posicion actual y debe reconfigurarse desde cero, causando un movimiento brusco e indeseado.
- **Estado en RAM perdido**: Deep Sleep solo preserva el dominio RTC (~8KB en SRAM). Todo el estado del sistema (buffer circular de pre-grabacion, configuracion de camara, estado de WiFi) se pierde y debe reconstruirse completamente.
- **Wake lento**: El wake desde Deep Sleep toma aproximadamente 1-2 segundos debido a la reinicializacion completa del sistema (bootloader, drivers, perifericos). Para una camara de seguridad, este retardo es critico: el evento PIR podria ocurrir durante el periodo de wake y no capturarse.

### Ventajas del Light Sleep

- **Wake rapido**: ~10ms desde que se detecta la fuente de wake hasta que el CPU esta ejecutando codigo. Esto permite capturar eventos PIR sin perdida de frames iniciales.
- **RAM preservada**: Todo el contenido de la SRAM (~512KB) se mantiene intacto. El buffer circular, la configuracion de la camara y el estado del sistema sobreviven al sleep.
- **GPIO wake funcional**: Las interrupciones externas (EXT0/EXT1) funcionan correctamente en Light Sleep. GPIO1 (PIR) y GPIO0 (BOOT) pueden despertar el sistema inmediatamente.
- **Restauracion rapida**: Solo es necesario re-inicializar perifericos especificos (WiFi, camara, SPI) en lugar de todo el sistema.

---

## Que se apaga en Light Sleep

| Periferico | Estado | Razon |
|---|---|---|
| WiFi Radio | APAGADO | Mayor consumidor de energia (~70-100mA) |
| UART (serial) | APAGADO | No necesario durante sleep |
| LEDC (PWM servo) | APAGADO | Periferico digital, se apaga con dominio digital |
| SPI (SD card) | APAGADO | No accesible durante sleep |
| Camara (sensor) | POWER DOWN | OV3660 entra en modo standby via pin XCLK |
| I2S | APAGADO | No utilizado activamente |
| Bluetooth | APAGADO | No utilizado en este proyecto |
| LED indicador | APAGADO | GPIO en estado hold |

---

## Que se mantiene

| Componente | Estado | Detalle |
|---|---|---|
| RTC Domain | ACTIVO | Timer wake, GPIO wake via RTC pins (GPIO0-21) |
| SRAM RTC | ACTIVO | ~512KB disponibles para estado del sistema |
| GPIO0/1 interrupciones | ACTIVAS | Configuradas como fuentes de wake |
| RTC Peripherals | ACTIVOS | Mantiene senales de wake y configuracion |
| Watchdog Timer | ACTIVO | Previene colgadas durante wake |

---

## Configuracion del sleep

### Inicializacion de wake sources

```cpp
// PIR en GPIO1: wake en flanco HIGH (movimiento detectado)
esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 1);

// Boton BOOT en GPIO0: wake en flanco LOW ( boton presionado)
esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);
```

### Apagar dominios innecesarios

```cpp
// Apagar dominio digital (ahorra ~5-8mA adicional)
esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
esp_sleep_pd_config(ESP_PD_DOMAIN_VDDSDIO, ESP_PD_OPTION_OFF);
```

### Entrar en sleep

```cpp
// Configurar pines antes de sleep
gpio_hold_en(GPIO_NUM_2);   // Mantener servo en posicion actual
gpio_hold_en(GPIO_NUM_47);  // Mantener LED apagado

// Entrar en light sleep
esp_light_sleep_start();

// El CPU se detiene aqui. La proxima linea se ejecuta tras wake.
```

---

## Flujo completo del sleep cycle

```
┌─────────────────────────────────────────────────────────┐
│                    loop() - Main Loop                    │
├─────────────────────────────────────────────────────────┤
│                                                         │
│  1. Detecta inactividad (timeout configurable)          │
│     - No hay clientes WiFi conectados                   │
│     - No hay grabacion activa                           │
│     - No hay interaccion HTTP reciente                  │
│                                                         │
│  2. Guarda estado en NVS                                │
│     - Angulo actual del servo                           │
│     - Modo de operacion                                 │
│     - Configuracion de resolucion                       │
│     - Flags de estado                                   │
│                                                         │
│  3. Destruye perifericos                                │
│     - ledc_stop() - Detiene PWM del servo               │
│     - esp_camera_deinit() - Apaga camara                │
│     - WiFi.disconnect() - Desconecta radio              │
│     - sd_card_spi->end() - Libera SPI                   │
│                                                         │
│  4. Configura GPIO hold                                 │
│     - gpio_hold_en() para mantener estados criticos     │
│                                                         │
│  5. Entra en light sleep                                │
│     - esp_light_sleep_start()                           │
│     - CPU detenido, solo RTC activo                     │
│                                                         │
├─────────────────────── SLEEP ───────────────────────────┤
│                                                         │
│  ~12-18mA consumo durante sleep                         │
│                                                         │
├─────────────────────── WAKE ────────────────────────────┤
│                                                         │
│  6. ISR detecta fuente de wake (GPIO1 o GPIO0)          │
│     - ISR ejecuta: flag = true;                         │
│     - CPU se reactiva (~10ms)                           │
│                                                         │
│  7. Restaura todo desde NVS                             │
│     - Recupera angulo servo, modo, config               │
│     - Re-inicializa camara (esp_camera_init)            │
│     - Re-inicia WiFi AP si necesario                    │
│     - Re-inicia SPI/SD si necesario                     │
│     - Reposiciona servo (movimiento suave)              │
│                                                         │
│  8. Continua en loop() principal                        │
│     - Procesa evento que desperto el sistema            │
│     - Retorna a estado AWAKE_IDLE                       │
│                                                         │
└─────────────────────────────────────────────────────────┘
```

---

## Consumo energetico

| Estado | Consumo | Descripcion |
|---|---|---|
| Active (grabando + WiFi) | ~80-120mA | Camara + WiFi AP + SD write |
| Active (solo camara) | ~60-80mA | Camara activa, WiFi OFF |
| Active (idle) | ~40-60mA | CPU activo, perifericos en espera |
| Light Sleep | ~12-18mA | Solo RTC domain activo |
| Deep Sleep (referencia) | ~5-10mA | No util para este proyecto |

### Impacto en autonomia

Con una bateria de 2000mAh:

| Modo | Duracion estimada |
|---|---|
| Grabando continuamente | ~17-25 horas |
| Idle con WiFi | ~33-50 horas |
| Light Sleep (mayoria del tiempo) | ~111-166 horas (4.6-6.9 dias) |
| Mixto (10% activo, 90% sleep) | ~50-75 horas (2-3 dias) |

---

## Diagrama de timing

```
Nivel de consumo (mA)
    │
120 ┤        ┌──────────────────┐
    │        │   ACTIVE         │
 80 ┤        │  (grabando)      │
    │        │                  │
 60 ┤    ┌───┘                  └───┐
    │    │ AWAKE_IDLE                │
 40 ┤    │                          │
    │    │                          │
 18 ┤────┘                          └──────── SLEEP
    │                                        │
 12 ┤                                        │
    │                                        │
  0 ┼────────┬────────┬────────┬────────┬───┬───► tiempo
         PIR      30s      Wake    30s    Wake
        event   timeout   event  timeout  event
```

### Timeline de un ciclo completo

```
t=0s      PIR detecta movimiento
t=0.01s   Wake desde light sleep (~10ms)
t=0.06s   Restauracion completa (~50ms desde wake)
t=0.06s   Camara operativa, grabando
t=0.56s   500ms de grabacion (pre-buffer + continuo)
t=30s     PIR LOW por 3 segundos (cooldown)
t=33s     Cerrar clip AVI, renombrar .tmp -> .avi
t=35s     Entrar en light sleep
```

---

## Consideraciones criticas

1. **GPIO hold**: Antes de entrar en sleep, usar `gpio_hold_en()` para mantener el servo en posicion y el LED apagado. Sin esto, los GPIO flotan y el servo puede moverse aleatoriamente.

2. **RTC pins exclusivos**: Solo GPIO0-21 pueden usarse como fuentes de wake en Light Sleep. GPIO47 (LED) no puede generar wake. Para el proyecto actual, esto no es problema ya que PIR (GPIO1) y BOOT (GPIO0) son RTC pins.

3. **Restauracion de camara**: `esp_camera_init()` toma ~30-50ms. Durante este tiempo no hay frames disponibles. El pre-buffer en PSRAM compensa esta latencia.

4. **Timer wake como respaldo**: Ademas de GPIO, configurar un timer wake cada 60 segundos para verificar estado del sistema y evitar sleep indefinido si el PIR falla.
