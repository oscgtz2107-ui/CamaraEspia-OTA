# Esquematico de Conexiones

> **Nota:** Este diagrama representa conexiones logicas, no disposicion fisica.
> Las posiciones relativas de los componentes no corresponden al layout real del PCB.

## Diagrama ASCII

```
                           +5V (boost J5019)
                             |
                    +--------+--------+
                    |                 |
               +----+----+      +----+----+
               | J5019   |      | Servo   |
               | Carga + |      | SG90/   |
               | Boost   |      | MG90S   |
               | 5V      |      |         |
               +----+----+      +----+----+
                    |                 |
              BAT+  |            VCC  |
                    |                 |
             +------+                 |
             |  LiPo                 |
             |  18650                |
             |  2500mAh              |
             +------+                 |
                    |                 |
                    +--------+--------+
                             |
                            GND (comun)
                             |
          +------------------+------------------+
          |                  |                  |
     +----+----+       +----+----+        +----+----+
     | PIR     |       | ESP32   |        | SD      |
     | HC-SR501|       | S3      |        | MicroSD |
     |         |       | N16R8   |        | SPI     |
     | VCC 5V--|---+   |         |        |         |
     | OUT-----|---|---|>GPIO1   |        | VCC 3.3 |
     | GND-----|---+   |         |        | CS  ---|--- GPIO21
     +---------+   |   |         |        | MOSI---|--- GPIO40
         |         |   |         |        | CLK ---|--- GPIO41
         |         |   |         |        | MISO---|--- GPIO42
         |         |   |         |        | GND ---|--- GND
         |         |   |         |        +---------+
         |         |   |         |
         |         |   |         |        +---------+
         |         |   |         |        | LED     |
         |         |   |         |        | Estado  |
         |         |   |         |        | (GPIO47)|
         |         |   |         |        | LED  ---|--- GPIO47
         |         |   |         |        | GND ---|--- GND
         |         |   |         |        +---------+
         |         |   |         |
         |         |   |         |        +---------+
         |         |   |         |        | Boton   |
         |         |   |         |        | BOOT    |
         |         |   |         |        | BTN  ---|--- GPIO0
         |         |   |         |        | GND ---|--- GND
         |         |   |         |        +---------+
         |         |   |         |
         |         |   |         |   +-----------------------------+
         |         |   |         |   | CAMARA OV3660 CSI           |
         |         |   |         |   |                             |
         |         |   |   XCLK--|---|--> GPIO15                   |
         |         |   |   SIOD--|---|--> GPIO4                    |
         |         |   |   SIOC--|---|--> GPIO5                    |
         |         |   |  VSYNC--|---|--> GPIO6                    |
         |         |   |   HREF--|---|--> GPIO7                    |
         |         |   |    D0  -|---|--> GPIO11                   |
         |         |   |    D1  -|---|--> GPIO9                    |
         |         |   |    D2  -|---|--> GPIO8                    |
         |         |   |    D3  -|---|--> GPIO10                   |
         |         |   |    D4  -|---|--> GPIO12                   |
         |         |   |    D5  -|---|--> GPIO18                   |
         |         |   |    D6  -|---|--> GPIO17                   |
         |         |   |    D7  -|---|--> GPIO16                   |
         |         |   |   PCLK -|---|--> GPIO13                   |
         |         |   |   PWDN -|---|--> (GND o NC)              |
         |         |   |   RESET-|---|--> 3.3V (pull-up)          |
         |         |   |    VCC -|---|--> 3.3V                     |
         |         |   |    GND -|---|--> GND                      |
         |         |   +-----------------------------+              |
         |         |   |                             |              |
         |         |   |   Divisor voltaje bateria   |              |
         |         |   |                             |              |
         |         |   |   BAT+ ---[100k]---+---[100k]--- GND     |
         |         |   |                   |                       |
         |         |   |                   +---------> GPIO3       |
         |         |   |                  (ADC1_CH2)               |
         |         |   |                                           |
         |         |   +-------------------------------------------+
         |         |
         |         +---> GND comun
         |
         +---> GND comun


=== DETALLE DIVISOR DE VOLTAJE ===

    BAT+ (3.0 - 4.2V)
      |
     [R1 = 100k]
      |
      +---------> GPIO3 (ADC1_CH2)
      |
     [R2 = 100k]
      |
     GND

    V_out = V_bat x R2 / (R1 + R2)
    V_out = V_bat / 2

    Rango ADC en GPIO3:
      - Bateria vacia (3.0V): 1.5V en GPIO3
      - Bateria llena (4.2V): 2.1V en GPIO3
      - Rango ADC (0-3.3V): siempre dentro de rango


=== ALIMENTACIONES ===

    +5V .............. Boost desde J5019 (servo, PIR)
    +3.3V ............ Regulador interno ESP32-S3 (camara, SD, PIR logico)
    BAT+ ............. Directo al divisor de voltaje (ADC)
    GND .............. Comun a todos los modulos (OBLIGATORIO)
```

## Resena de conexiones

| Componente | Alimentacion | Pin Datos | Protocolo |
|---|---|---|---|
| PIR HC-SR501 | 5V boost | GPIO1 (OUT) | Digital INPUT (HIGH = movimiento) |
| Servo SG90/MG90S | 5V boost | GPIO2 (PWM) | LEDC 50Hz, 16-bit |
| Camara OV3660 | 3.3V interno | 13 pines CSI | Parallel + I2C (config) |
| SD MicroSD | 3.3V interno | GPIO21 (CS), 40, 41, 42 | SPI |
| Boton BOOT | pull-up interno | GPIO0 | Digital INPUT, active LOW |
| LED estado | 3.3V interno | GPIO47 | Digital OUTPUT |
| Battery ADC | BAT+ directo | GPIO3 (ADC1_CH2) | ADC 12-bit |

## Pines no utilizados disponibles para expansion

Los siguientes GPIO estan libres y pueden usarse para futuros componentes:

- GPIO14 (input-only)
- GPIO38, GPIO39, GPIO48
- GPIO2 (reutilizable si servo no esta conectado)
