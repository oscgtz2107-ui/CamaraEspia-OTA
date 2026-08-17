# serial_log.ps1 - Monitor serial del ESP32-S3
param([int]$Baud = 115200)
$port = Get-SerialPort | Select-Object -First 1
if ($port) {
    Write-Host "Conectando a $($port.PortName) a $Baud baudios..." -ForegroundColor Green
    & pio device monitor --port $port.PortName --baud $Baud
} else {
    Write-Host "No se detecto puerto serial. Conecta el ESP32-S3." -ForegroundColor Red
}
