# Mapa de Pines - ESP32-S3 N16R8

## Resumen

| Categoria | Cantidad | Pines |
|---|---|---|
| **USADO** | 20 | 0, 1, 2, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 15, 16, 17, 18, 21, 40, 41, 42, 47 |
| **LIBRE** | 5 | 3, 14, 38, 39, 48 |
| **PROHIBIDO** | 16 | 19, 20, 26-37, 43-44, 45-46 |
| **TOTAL** | 49 | GPIO0 - GPIO48 |

---

## Tabla Completa de GPIO

| GPIO | Funcion | Direccion | Tipo | Nota |
|---|---|---|---|---|
| **0** | Boton BOOT | INPUT | Digital, pull-up interno | **USADO** - Strapping pin. Active LOW. En boot: mantener LOW para entrar en modo download. Ver nota abajo. |
| **1** | PIR HC-SR501 | INPUT | Digital | **USADO** - Salida 3.3V logica del PIR. HIGH = movimiento detectado. |
| **2** | Servo SG90/MG90S | OUTPUT | PWM (LEDC 50Hz) | **USADO** - Seccion PWM controlada por LEDC channel. 50Hz, resolucion 16-bit. |
| **3** | Battery ADC | INPUT | Analogico (ADC1_CH2) | **LIBRE** - Divisor 100k+100k desde BAT+. Evitar ruido digital. Ver nota abajo. |
| **4** | Camara SIOD (I2C SDA) | BIDIR | I2C | **USADO** - Bus I2C para configuracion de registros OV3660. |
| **5** | Camara SIOC (I2C SCL) | OUTPUT | I2C | **USADO** - Bus I2C para configuracion de registros OV3660. |
| **6** | Camara VSYNC | INPUT | Digital | **USADO** - Sincronizacion vertical del sensor OV3660. |
| **7** | Camara HREF | INPUT | Digital | **USADO** - Referencia horizontal (indica linea activa). |
| **8** | Camara D2 | INPUT | Digital | **USADO** - Bit 2 del bus paralelo de datos. |
| **9** | Camara D1 | INPUT | Digital | **USADO** - Bit 1 del bus paralelo de datos. |
| **10** | Camara D3 | INPUT | Digital | **USADO** - Bit 3 del bus paralelo de datos. |
| **11** | Camara D0 | INPUT | Digital | **USADO** - Bit 0 del bus paralelo de datos. |
| **12** | Camara D4 | INPUT | Digital | **USADO** - Bit 4 del bus paralelo de datos. |
| **13** | Camara PCLK | INPUT | Digital | **USADO** - Reloj de pixel del sensor. |
| **14** | (disponible) | INPUT | Input-only | **LIBRE** - Solo puede usarse como INPUT. No puede generar output. |
| **15** | Camara XCLK | OUTPUT | Digital | **USADO** - Reloj maestro para el sensor (20MHz tipico). |
| **16** | Camara D7 | INPUT | Digital | **USADO** - Bit 7 del bus paralelo de datos. |
| **17** | Camara D6 | INPUT | Digital | **USADO** - Bit 6 del bus paralelo de datos. |
| **18** | Camara D5 | INPUT | Digital | **USADO** - Bit 5 del bus paralelo de datos. |
| **19** | USB D- | USB | USB OTG | **PROHIBIDO** - Pin USB nativo. Usado por USB OTG. No tocar. |
| **20** | USB D+ | USB | USB OTG | **PROHIBIDO** - Pin USB nativo. Usado por USB OTG. No tocar. |
| **21** | SD CS (chip select) | OUTPUT | SPI | **USADO** - Select de chip para MicroSD en bus SPI. |
| **22** | (sin acceso) | - | - | No disponible en包裝 QFN (no tiene pad fisico). |
| **23** | (sin acceso) | - | - | No disponible en包裝 QFN (no tiene pad fisico). |
| **24** | (sin acceso) | - | - | No disponible en包裝 QFN (no tiene pad fisico). |
| **25** | (sin acceso) | - | - | No disponible en包裝 QFN (no tiene pad fisico). |
| **26** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **27** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **28** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **29** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **30** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **31** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **32** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **33** | Flash octal SPI | SPI | Octal PSRAM | **PROHIBIDO** - Pin de bus octal PSRAM. Usado internamente por el chip. |
| **34** | Flash octal SPI | SPI | Octal Flash | **PROHIBIDO** - Pin de bus octal Flash. Usado internamente por el chip. |
| **35** | Flash octal SPI | SPI | Octal Flash | **PROHIBIDO** - Pin de bus octal Flash. Usado internamente por el chip. |
| **36** | Flash octal SPI | SPI | Octal Flash | **PROHIBIDO** - Pin de bus octal Flash. Usado internamente por el chip. |
| **37** | Flash octal SPI | SPI | Octal Flash | **PROHIBIDO** - Pin de bus octal Flash. Usado internamente por el chip. |
| **38** | (disponible) | I/O | Digital | **LIBRE** - Pin GPIO general, disponible para uso futuro. |
| **39** | (disponible) | I/O | Digital | **LIBRE** - Pin GPIO general, disponible para uso futuro. |
| **40** | SD MOSI | OUTPUT | SPI | **USADO** - Master Out Slave In para bus SPI de MicroSD. |
| **41** | SD CLK | OUTPUT | SPI | **USADO** - Reloj del bus SPI de MicroSD. |
| **42** | SD MISO | INPUT | SPI | **USADO** - Master In Slave Out del bus SPI de MicroSD. |
| **43** | UART0 TX | OUTPUT | UART | **PROHIBIDO** - TX del UART0 (consola de debug). Usado por USB-Serial. |
| **44** | UART0 RX | INPUT | UART | **PROHIBIDO** - RX del UART0 (consola de debug). Usado por USB-Serial. |
| **45** | Strapping | - | Digital | **PROHIBIDO** - Pin de strapping. Configura modo de boot. No usar. |
| **46** | Strapping | - | Digital | **PROHIBIDO** - Pin de strapping. Configura modo de boot. No usar. |
| **47** | LED estado | OUTPUT | Digital | **USADO** - LED indicador de estado. Ver nota abajo. |
| **48** | (disponible) | I/O | Digital | **LIBRE** - Pin GPIO general, disponible para uso futuro. |

---

## Notas Importantes

### GPIO0 (BOOT)

- **Strapping pin**: En el momento del reset, el nivel de GPIO0 determina si el ESP32-S3 entra en modo normal o en modo download.
- Si GPIO0 esta en LOW durante el reset -> modo download (para flashear).
- Si GPIO0 esta en HIGH durante el reset -> modo normal (arranca firmware).
- El boton BOOT generalmente esta conectado entre GPIO0 y GND. Pulsarlo pone GPIO0 en LOW.
- Para flashear: mantener pulsado BOOT, luego reset, luego liberar BOOT.
- En codigo: configurar como INPUT con pull-up interno. Leer con `digitalRead(0)`.

### GPIO3 (ADC Battery)

- **ADC1_CH2**: Este canal ADC esta en el grupo ADC1, que es compatible con WiFi (no todos los canales lo son).
- **Evitar ruido digital**: No usar este pin como GPIO digital. Solo como entrada analogica.
- **Rango recomendado**: 0-3.3V. Con divisor 100k+100k, el rango efectivo es 1.5V-2.1V (bateria 3.0V-4.2V).
- **Calibracion**: Usar `esp_adc_cal` o `adc_cali` para obtener lecturas precisas.
- **Atenuacion**: Usar atenuacion de 11dB para cubrir el rango completo.

### GPIO47 (LED Estado)

- En algunos modulos ESP32-S3 (DevKit), GPIO47 esta conectado a un LED onboard.
- En otros modulos puede no tener LED conectado. Verificar con el fabricante.
- Si se usa como LED: configurar como OUTPUT, HIGH = LED encendido.
- Si el modulo no tiene LED: este pin queda libre.

### GPIO14 (Input-Only)

- Este pin solo puede usarse como **entrada**. No puede configurarse como OUTPUT.
- Util para sensores digitales que solo leen datos.
- No puede usar pull-up/down interno de software de la misma forma que otros GPIO.

### Pines 22-25

- Estos GPIO no estan expuestos fisicamente en el包裝 QFN del ESP32-S3. No tienen pad de conexion.
- No es posible usarlos sin modificar el chip.
