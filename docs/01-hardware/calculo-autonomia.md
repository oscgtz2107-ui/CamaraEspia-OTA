# Calculo de Autonomia

## Parametros de la Bateria

| Parametro | Valor |
|---|---|
| Tipo | 18650 Li-Ion |
| Capacidad nominal | 2500 mAh |
| Voltaje nominal | 3.7V |
| Energia total | 9.25 Wh |
| Voltaje carga completa | 4.2V |
| Voltaje minimo seguro | 3.0V |
| Energia util | 2500mAh x (4.2V - 3.0V) / 2 x 2 = ~3.75Wh util* |

*Nota: La energia util real depende del corte de descarga. Con corte en 3.3V, la energia util es menor.*

## Consumo por Estado

| Estado | Corriente tipica | Descripcion |
|---|---|---|
| Light Sleep | 12 - 18 mA | ESP32-S3 suspendido, CPU apagada, RAM en self-refresh. GPIO wake habilitado (PIR). PIR activo (~0.065mA). LED apagado. Camara apagada. SD en standby. |
| Awake idle (WiFi AP) | 80 - 120 mA | ESP32 despierto, WiFi en modo AP (sin clientes conectados), procesando logica basica, sin stream activo. |
| Streaming MJPEG | 180 - 220 mA | WiFi TX activo (transmitiendo frames), camara OV3660 activa, procesamiento JPEG, LEDC activo (servo en posicion). |
| Grabando a SD | 250 - 300 mA | Streaming + escritura en SD via SPI. El SD SPI consume ~50mA adicionales durante escritura. |
| Grabando continuo + streaming | 300 - 350 mA | Todo activo simultaneamente: camara, WiFi, SD, servo. Pico durante transmision WiFi. |

### Desglose de componentes

| Componente | Consumo minimo | Consumo maximo | Notas |
|---|---|---|---|
| ESP32-S3 (core) | 8 mA (sleep) | 350 mA (todos activos) | Varia con frecuencia CPU y perifericos |
| WiFi TX | 20 mA | 240 mA | Depende de potencia TX y distance |
| WiFi RX | 8 mA | 90 mA | |
| Camara OV3660 | 40 mA | 120 mA | Activa en streaming, ~0.5mA en standby |
| PIR HC-SR501 | 0.065 mA | 3 mA | Reposo muy bajo, picos al detectar |
| Servo SG90 | 0 mA (sleep) | 750 mA (pico) | ~100mA sostenido, 0 en sleep |
| SD MicroSD SPI | 0.1 mA | 100 mA | Standby muy bajo, ~50-100mA durante escritura |
| LED estado | 0 mA | 20 mA | Depende del LED y resistencia |

## Escenarios de Uso

### Escenario 1: Vigilancia Pasiva

**Comportamiento:**
- 95% del tiempo en Light Sleep (12mA promedio).
- 5% del tiempo despierto procesando eventos o manteniendo WiFi AP.

**Calculo:**
```
Tiempo en sleep:  0.95 x T_total  a  12 mA
Tiempo awake:     0.05 x T_total  a  100 mA (WiFi AP idle)

Consumo promedio = (0.95 x 12) + (0.05 x 100)
                 = 11.4 + 5.0
                 = 16.4 mA

Autonomia = 2500 mAh / 16.4 mA = 152 horas = 6.3 dias
```

**Estimacion realista: 5 - 7 dias**

La variacion depende de:
- Frecuencia de deteccion de movimiento (cada deteccion despierta el ESP32).
- Tiempo que el ESP32 permanece despierto despues de cada evento.
- Calidad de la bateria (capacidad real vs nominal).

---

### Escenario 2: Solo Movimiento (Trigger Ocasional)

**Comportamiento:**
- 90% del tiempo en Light Sleep (12mA).
- 10% del tiempo: detectar movimiento, capturar imagen, enviar por WiFi.
- Cada evento dura ~2-5 segundos.

**Calculo:**
```
Tiempo en sleep:    0.90 x T_total  a  12 mA
Tiempo processing:  0.10 x T_total  a  200 mA (streaming corto)

Consumo promedio = (0.90 x 12) + (0.10 x 200)
                 = 10.8 + 20.0
                 = 30.8 mA

Autonomia = 2500 mAh / 30.8 mA = 81 horas = 3.4 dias
```

**Estimacion realista: 2 - 4 dias**

Si los eventos son muy esporadicos (una vez por hora), la autonomia se acerca al escenario 1.
Si hay muchos eventos (cada 30 segundos), baja significativamente.

---

### Escenario 3: Grabacion Continua

**Comportamiento:**
- 100% del tiempo activo: camara + WiFi + SD + servo.
- Sin sleep. Todo funciona de forma continua.

**Calculo:**
```
Consumo promedio = 320 mA (grabando + streaming + servo)

Autonomia = 2500 mAh / 320 mA = 7.8 horas
```

**Estimacion realista: 6 - 9 horas**

Con una bateria de mayor capacidad (por ejemplo, dos 18650 en paralelo = 5000mAh), se duplicaria a 12-18 horas.

---

## Resumen de Escenarios

| Escenario | Consumo promedio | Autonomia estimada | Uso recomendado |
|---|---|---|---|
| Vigilancia pasiva | ~16 mA | 5 - 7 dias | Monitoreo con alertas por movimiento |
| Trigger por movimiento | ~31 mA | 2 - 4 dias | Captura bajo demanda |
| Grabacion continua | ~320 mA | 6 - 9 dias | Uso intensivo, bateria externa recomendada |

## Consideraciones Adicionales

### Factores que reducen autonomia

- **Temperatura**: A bajas temperaturas (<0C), la capacidad de la bateria se reduce significativamente. A 0C puede perder 20% de capacidad.
- **Calidad de bateria**: Baterias viejas o de mala calidad pueden tener solo 60-70% de su capacidad nominal.
- **Distancia WiFi**: A mayor distancia del router, mayor potencia TX = mayor consumo.
- **Frecuencia de grabacion**: Mas frames por segundo = mas datos = mas tiempo activo del WiFi.
- **Modo WiFi**: AP consume mas que STA (cliente). Si el ESP32 se conecta a un AP existente, puede ahorrar algo de energia.

### Optimizaciones de software

- Usar **Light Sleep** con GPIO wake como metodo principal de ahorro.
- Reducir la resolucion de la camara cuando no se necesita maxima calidad.
- Usar **deep sleep** si no se necesita mantener estado (pero pierde RAM).
- Apagar WiFi cuando no se esta transmitiendo (redundante con sleep).
- Usar el timer del ESP32 para despertar periodicamente y revisar el PIR.

### Monitorizacion de bateria

El pin GPIO3 (ADC) permite monitorear el nivel de bateria en tiempo real:

```
Voltaje en GPIO3    Voltaje Bateria    Nivel    Accion
-----------         ---------------    -----    ------
>= 2.10V            >= 4.2V            100%     Carga completa
1.85V               3.7V               ~50%     Normal
1.65V               3.3V               ~20%     Alertar
1.50V               3.0V               ~0%      Apagar / Deep sleep
```

**Recomendacion**: Programar una interrupcion o revisar periodicamente el ADC. Si VBAT < 3.3V, entrar en deep sleep para proteger la bateria.
