# Fase 0: Preparacion de Hardware

## Objetivo
Verificar y preparar todo el hardware fisico antes de escribir cualquier codigo. Cada paso produce un resultado medible con herramientas basicas (multimetro, cables jumper).

## Checklist

### Verificacion inicial del paquete
- [ ] Verificar contenido del paquete ESP32-S3 (placa, cable USB, pines separados)
- [ ] Medir voltaje de batería 18650 (debe estar >3.5V para operacion segura)
- [ ] Si voltaje < 3.0V: cargar batería antes de continuar
- [ ] Si voltaje < 2.5V: bateria dañada, reemplazar

### Alimentacion con J5019
- [ ] Conectar J5019 a bateria 18650
- [ ] Medir 5V en pin VOUT del J5019 con multimetro
- [ ] Si no hay 5V: verificar polarity de bateria, verificar soldaduras del J5019

### Divisor de voltaje para ADC
- [ ] Medir voltaje en nodo divisor de voltaje (conexion a GPIO3)
- [ ] Deberia ser ~2.1V con batería llena (4.2V)
- [ ] Verificar formula: V_adc = V_bat * (R2 / (R1 + R2))
- [ ] Si voltaje incorrecto: verificar valores de resistencias R1 y R2

### Modulo SD
- [ ] Soldar módulo SD en protoboard con pines correctos
- [ ] Verificar continuidad de soldaduras con multimetro (modo continuidad)
- [ ] Pines a verificar:
  - CS a GPIO21
  - MOSI a GPIO40
  - SCLK a GPIO41
  - MISO a GPIO42
  - VCC a 3.3V
  - GND a GND
- [ ] Insertar tarjeta SD (FAT32, <=32GB recomendado)
- [ ] No insertar tarjeta con fuerza excesiva

### Sensor PIR HC-SR501
- [ ] Conectar PIR: VCC a 5V, GND a GND, OUT a GPIO1
- [ ] Ajustar sensibilidad PIR (potenciómetro Sx, position central ~50%)
- [ ] Ajustar tiempo de retencion (potenciómetro Tx, position minima ~3s)
- [ ] Verificar con LED temporal: mover mano frente al sensor, LED debe encender

### Servo SG90
- [ ] Conectar servo: cable señal a GPIO2, VCC a 5V, GND a GND
- [ ] Verificar que el servo no mueve con laConexion
- [ ] Test fisico: mover brazo manualmente, debe tener resistencia

### Camara CSI OV3660
- [ ] Verificar que la camara CSI esta bien conectada (conector FPC abierto, insertar, cerrar)
- [ ] Conector FPC debe estar perpendicular a la placa
- [ ] No forzar el conector FPC

### LED estado
- [ ] Conectar LED estado a GPIO47 con resistor 220 ohm en serie
- [ ] Verificar polarity del LED (anodo al resistor, catodo a GND)

### Boton BOOT
- [ ] Verificar pull-up interno del boton BOOT
- [ ] Medir con multimetro: debe leer HIGH cuando no presionado
- [ ] Al presionar: debe leer LOW

### Prueba de alimentacion completa
- [ ] Conectar todos los modulos simultaneamente:
  - Camara CSI
  - Modulo SD
  - Servo
  - PIR
  - LED
- [ ] Medir corriente total en modo idle (sin transmision WiFi activa)
  - Esperado: 80-120mA
  - Si > 200mA: buscar cortocircuito o modulo defectuoso
- [ ] Verificar que J5019 soporta carga mientras esta encendido (USB conectado)
- [ ] Verificar que no hay calentamiento excesivo en ningun componente

## Herramientas necesarias
- Multimetro digital
- Cables jumper M-M y M-H
- Protoboard
- Soldador y estaño
- Lupa o lupita para verificar soldaduras

## Criterio de aprobacion
Todos los items marcados. El hardware esta fisicamente listo para receber firmware.
