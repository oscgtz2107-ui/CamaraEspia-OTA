# minify_pwa.ps1 - Copia PWA a firmware/data/ para SPIFFS
$source = Join-Path $PSScriptRoot "..\pwa"
$dest = Join-Path $PSScriptRoot "..\firmware\data"

# Limpiar destino
if (Test-Path $dest) { Remove-Item -Recurse -Force $dest }
New-Item -ItemType Directory -Force -Path $dest | Out-Null

# Copiar archivos
Copy-Item -Recurse "$source\*" $dest

Write-Host "PWA copiada a firmware/data/" -ForegroundColor Green
Write-Host "Ejecuta: pio run -t uploadfs" -ForegroundColor Yellow
