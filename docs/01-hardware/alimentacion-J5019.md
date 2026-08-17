# Alimentacion - Modulo J5019

## Diagrama de Bloques

```
  USB-C (5V / 2A)
       |
       v
  +-----------+
  |   J5019   |
  |  Carga +  |
  |  Boost    |
  |  5V out   |
  +-----------+
       |
       +---[Carga LiPo]----> Bateria 18650 (3.7V nominal, 2500mAh)
       |
       +---[Boost 5V]------> Rail +5V
                                |
                                +-----> Servo SG90/MG90S (VCC)
                                |
                                +-----> PIR HC-SR501 (VCC)
                                |
                                +-----> (otros componentes 5V)


  Bateria 18650 (3.0 - 4.2V)
       |
       +---[Divisor 100k+100k]---> GPIO3 (ADC, voltaje / 2)
       |
       +---[GND comun]-----------> ESP32 GND, PIR GND, Servo GND, SD GND
```

## Puntos de Medicion

| Punto | Voltaje | Origen | Uso |
|---|---|---|---|
| VBAT | 3.0 - 4.2V | Bateria directa | Divisor ADC (GPIO3) |
| VOUT 5V | ~5.0V | Boost J5019 | Servo, PIR, placa si necesita 5V |
| V3V3 | 3.3V | Regulador interno ESP32-S3 | Camara OV3660, SD, logica |
| GND | 0V | Comun | Referencia para todos los modulos |

## Divisor de Voltaje - Detalle

### Esquematico

```
    BAT+ (3.0 - 4.2V)
      |
     [R1 = 100k]  (1% tolerance recomendado)
      |
      +-----> GPIO3 (ADC1_CH2)
      |
     [R2 = 100k]  (1% tolerance recomendado)
      |
     GND
```

### Formulas

```
V_out = V_bat * R2 / (R1 + R2)

Con R1 = R2 = 100k:
  V_out = V_bat / 2

Valores tipicos:
  Bateria vacia  (3.0V)  ->  1.50V en GPIO3
  Bateria media  (3.7V)  ->  1.85V en GPIO3
  Bateria llena  (4.2V)  ->  2.10V en GPIO3
```

### Corriente de fuga del divisor

```
I_divider = V_bat / (R1 + R2)

Con bateria llena (4.2V):
  I = 4.2V / 200k = 21 uA

Con bateria vacia (3.0V):
  I = 3.0V / 200k = 15 uA

-> Despreciable (no afecta autonomia de forma medible)
```

### Capacitor de filtro (recomendado)

Colocar un capacitor de 100nF entre GPIO3 y GND, lo mas cerca posible del pin.

- Filtra ruido de alta frecuencia.
- Impedancia de entrada del ADC: usar atenuacion de 11dB.
- Si se usa con WiFi activo: considerar un capacitor de 10uF adicionales.

## Advertencias

### GND Comun - OBLIGATORIO

**Todos los modulos deben compartir el mismo GND.** Si un modulo tiene un GND separado, los niveles logicos no seran consistentes y podrian danar componentes.

Conexiones GND obligatorias:
- J5019 GND -> ESP32-S3 GND
- ESP32-S3 GND -> PIR HC-SR501 GND
- ESP32-S3 GND -> Servo SG90/MG90S GND (cable marron)
- ESP32-S3 GND -> SD MicroSD GND
- ESP32-S3 GND -> Camara OV3660 GND

**Regla**: Todos los GND se conectan a un solo punto (topologia estrella) o a un plano comun.

### Servo - NO usar 3.3V del ESP32

El servo SG90/MG90S requiere **5V** para funcionar correctamente. La alimentacion de 3.3V del ESP32-S3:
- No es suficiente para mover el servo.
- Podria dañar el regulador de 3.3V del ESP32 por exceso de corriente.
- El servo consume 100-750mA segun carga (el ESP32 solo puede entregar ~400mA desde su regulador).

**Conexion correcta del servo:**
- Cable rojo (VCC) -> +5V (boost J5019)
- Cable marron (GND) -> GND comun
- Cable naranja (senal) -> GPIO2 del ESP32-S3 (senal PWM 3.3V, OK para input del servo)

### PIR HC-SR501 - Alimentacion

- Rango de alimentacion: 5V - 20V. Funciona perfecto con 5V del boost.
- Consumo tipico: ~65uA en reposo, ~3mA activo.
- Salida logica: 3.3V (compatibile con GPIO del ESP32-S3).
- El PIR tiene un regulador interno, por lo que 5V es seguro.

**Ajuste del PIR:**
- Potenciometro de sensibilidad: ajustar segun necesidad.
- Potenciometro de tiempo: duracion del pulso de salida.
- Jumper "H/L": dejar en H (retrigger) para mantener HIGH mientras detecta movimiento.

### Corriente del Boost J5019

- Corriente maxima del boost: 2A tipico (varia segun modelo exacto del J5019).
- Pico del servo: hasta 750mA al arrancar (sin carga pesada ~200mA).
- Consumo ESP32-S3 + SD: ~300mA max.
- **Total maximo**: ~1050mA (servo pico + ESP32 + SD) < 2A -> OK.

### Proteccion del Bateria

- El J5019 incluye proteccion contra sobrecarga (carga).
- El ESP32-S3 puede monitorear el voltaje via GPIO3 (ADC).
- Configurar alerta en software si VBAT < 3.3V (aprox. 1.65V en GPIO3) para evitar descarga profunda.
- La bateria 18650 tiene proteccion integrada contra sobredescarga (3.0V corte tipico).

## Tabla de Voltajes por Nodo

```
Nodo            Voltaje       Fuente              Componentes conectados
-----------     ---------     ------------------   ----------------------
VBAT            3.0 - 4.2V    Bateria directa      Divisor ADC (100k+100k)
VOUT_5V         ~5.0V         Boost J5019          Servo VCC, PIR VCC
V3V3            3.3V          Regulador ESP32      Camara, SD, PIR logico
GPIO3_div       1.5 - 2.1V   Divisor              ADC del ESP32
GND             0V            Comun                Todos los modulos
```
