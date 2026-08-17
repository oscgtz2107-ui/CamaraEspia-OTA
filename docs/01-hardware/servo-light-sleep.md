# Servo en Light Sleep - Comportamiento y Restauracion

## Problema

Cuando el ESP32-S3 entra en Light Sleep, el dominio digital se apaga parcialmente. El periferico LEDC (Hardware PWM) deja de generar la senal de 50Hz necesaria para controlar el servo. Sin senal PWM, el servo pierde el control activo del angulo.

## Comportamiento del Servo sin Senal PWM

### Que ocurre electricamente

1. La senal PWM en GPIO2 desaparece (pin queda en estado flotante o LOW).
2. El controlador interno del servo (circuito en el PCB del servo) deja de recibir pulsos.
3. El motor DC interno se desenergiza.
4. El servo **MANTIENE su posicion** por la friccion mecanica de los engranajes plasticos (SG90) o metalicos (MG90S).

### Engranajes plasticos (SG90)

- La friccion es suficiente para mantener la posicion con **carga ligera** (una camara OV3660 pesa ~3g).
- Si hay vibrations o carga pesada, el servo puede moverse gradualmente.
- No hay dano al servo por mantener posicion sin senal.

### Engranajes metalicos (MG90S)

- Mayor friccion, mejor retencion de posicion.
- Recomendado si la camara tiene lente o soporte adicional con peso significativo.

### Duracion sin senal

- El servo puede mantener posicion indefinidamente mientras no haya fuerza externa significativa.
- No hay consumo electrico adicional (motor desenergizado).
- No hay dano al servo.

## Flujo de Wake-up (Restauracion)

```
Estado: LIGHT SLEEP (GPIO2 sin senal, servo en posicion por friccion)
         |
         v
[PIR detecta movimiento]  O  [Boton BOOT pulsado]
         |
         v
GPIO1 (PIR) o GPIO0 (BOT) genera interrupcion de wake
         |
         v
Hardware despierta el ESP32-S3 desde Light Sleep
         |
         v
setup() se ejecuta (o funcion de restauracion especifica)
         |
         v
1. Restaurar canal LEDC en GPIO2
   -> ledcSetup(channel, freq=50, resolution=16)
   -> ledcAttachPin(GPIO2, channel)
         |
         v
2. Leer angulo desde NVS (almacenado antes de dormir)
   -> nvs_get_u16("servo_angle", &angle)
         |
         v
3. Aplicar PWM con el angulo restaurado
   -> ledcWrite(channel, angle_to_duty(angle))
         |
         v
4. Esperar estabilizacion (~100ms)
   delay(100)
         |
         v
Servo operativo nuevamente. Angulo restaurado.
```

## Flujo de Entrar en Sleep

```
Estado: OPERATIVO (servo en angulo X, LEDC activo)
         |
         v
1. Leer angulo actual desde LEDC
   -> current_angle = ledcRead(channel)
         |
         v
2. Guardar angulo en NVS (si no estaba ya guardado)
   -> nvs_set_u16("servo_angle", current_angle)
   -> nvs_commit()
         |
         v
3. Destruir canal LEDC
   -> ledcDetachPin(GPIO2)
   -> ledcDestroy(channel)
   (GPIO2 queda en estado flotante o HIGH/LOW configurable)
         |
         v
4. Configurar wake source (si no estaba configurado)
   -> esp_sleep_enable_gpio_wakeup() para GPIO1 (PIR)
   -> esp_sleep_enable_ext0_wakeup() para GPIO0 (BOT) si aplica
         |
         v
5. Entrar en Light Sleep
   -> esp_light_sleep_start()
         |
         v
Estado: LIGHT SLEEP (servo en posicion por friccion)
```

## Codigo de Referencia (ESP-IDF / Arduino)

### Guardar angulo y entrar en sleep

```cpp
#include "nvs_flash.h"
#include "nvs.h"
#include "driver/ledc.h"
#include "esp_sleep.h"

#define SERVO_GPIO      2
#define SERVO_CHANNEL   LEDC_CHANNEL_0
#define SERVO_FREQ_HZ   50
#define SERVO_RESOLUTION LEDC_TIMER_16_BIT

void guardarAnguloYDormir() {
    // Leer angulo actual
    uint32_t current_duty = ledcRead(SERVO_CHANNEL);

    // Guardar en NVS
    nvs_handle_t nvs;
    nvs_open("servo", NVS_READWRITE, &nvs);
    nvs_set_u16(nvs, "servo_duty", (uint16_t)current_duty);
    nvs_commit(nvs);
    nvs_close(nvs);

    // Destruir canal LEDC
    ledcDetachPin(SERVO_GPIO);

    // Configurar wake por GPIO1 (PIR)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_1, 1);

    // Entrar en light sleep
    esp_light_sleep_start();
}
```

### Restaurar angulo al despertar

```cpp
void restaurarServo() {
    // Restaurar canal LEDC
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .duty_resolution = SERVO_RESOLUTION,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = SERVO_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ch_conf = {
        .gpio_num = SERVO_GPIO,
        .speed_mode = LEDC_HIGH_SPEED_MODE,
        .channel = SERVO_CHANNEL,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&ch_conf);

    // Leer angulo desde NVS
    nvs_handle_t nvs;
    uint16_t saved_duty = 0;
    nvs_open("servo", NVS_READONLY, &nvs);
    nvs_get_u16(nvs, "servo_duty", &saved_duty);
    nvs_close(nvs);

    // Aplicar angulo restaurado
    if (saved_duty > 0) {
        ledcWrite(SERVO_CHANNEL, saved_duty);
        delay(100);  // Esperar estabilizacion del servo
    }
}
```

## Diagrama Temporal

```
Tiempo -->
|
|   OPERANDO          |  SLEEP  |   DESPIERTO (restaurado)
|   (servo activo)    |         |   (servo restaurado)
|                     |         |
|---LEDC ON---\       |         |    /---LEDC ON---
|              \      |         |   /
|   PWM 50Hz    \     |  No     |  /   PWM 50Hz
|   en GPIO2     \    |  PWM    | /    en GPIO2
|                  \   |         |/
|                   \  |         |
|                    \ |         |
|   GPIO2: ~~~~~~~~\_ |_________/~~~~~~~~
|                     |         |
|   Servo:  ANGULO X  | X (fri.)| ANGULO X (restaurado)
|                     |         |
|   Corriente: 100mA+ | 0 mA   | 100mA+
|                     |         |
|   Evento:           | PIR     | setup()
|   (cualquiera)      | wake    | restaura LEDC
|                     |         |
v                     v         v
```

## Advertencias

### Carga Pesada en el Servo

Si se monta un lente pesado o cualquier carga significativa sobre el servo:

- La friccion de los engranajes plasticos (SG90) **NO es suficiente** para mantener la posicion.
- El servo podria moverse gradualmente por gravedad durante el sleep.
- **Soluciones:**
  1. Usar **MG90S** (engranajes metalicos) en lugar de SG90.
  2. Agregar un **mecanismo de traba mecanica** externo (resorte, cierre).
  3. Usar un servo con funcion **brake** (freno), aunque no es comun en servos hobby.
  4. Diseñar el soporte de la camara para que la gravedad no aplique torque al servo (eje vertical o equilibrado).

### Vibraciones

- Vibraciones externas pueden causar que el servo se desplace ligeramente durante el sleep.
- Si la precision del angulo es critica, considerar un mecanismo de posicionamiento con feedback (encoder), aunque esto增加了 complejidad y consumo.

### Angulo Inicial

- Si el NVS no tiene un angulo guardado (primera ejecucion), el servo quedara en la posicion por defecto (generalmente 0 grados o la posicion donde estaba al perder senal).
- Inicializar el NVS con un angulo razonable en el primer boot.

### GPIO2 durante Sleep

- Durante el sleep, GPIO2 queda en estado flotante.
- Si esto causa problemas (ruido, consumo parasitario), se puede configurar el pin con pull-up o pull-down interno antes de dormir.
- La resistencia pull-up/down interna del ESP32 es ~45k, suficiente para mantener el pin en estado conocido.
