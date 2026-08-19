# CamaraEspia-OTA

Repositorio **público** de releases y actualizaciones OTA de CamaraEspia.
El código vive en el repo privado [CamaraEspia](https://github.com/oscgtz2107-ui/CamaraEspia);
aquí solo se compila y se publica.

## Cómo funciona

```
push a main (repo privado)
        │  toca firmware/ o android/
        ▼
dispatch.yml ──repository_dispatch──▶ build.yml (este repo)
                                          │
                    ┌─────────────────────┼─────────────────────┐
                    ▼                     ▼                     ▼
              build-firmware          build-apk               version
              (PlatformIO)         (Gradle + firma)      (única, compartida)
                    └─────────────────────┬─────────────────────┘
                                          ▼
                                       release
                              GitHub Release + version.json
                                     (rama gh-pages)
```

Todo es automático: basta con hacer push. **Lo único manual es subir la versión**
en `FIRMWARE_VERSION` (`firmware/src/config.h`) cuando quieras publicar una nueva.

## Endpoint que consultan los dispositivos

`https://oscgtz2107-ui.github.io/CamaraEspia-OTA/version.json`

```json
{
  "version": "1.2.1",
  "sha": "abc1234",
  "date": "2026-08-18T21:00:00Z",
  "firmware_url": "https://github.com/.../CamaraEspia-v1.2.1.bin",
  "sha256_url":   "https://github.com/.../firmware.sha256",
  "apk_url":      "https://github.com/.../CamaraEspia-v1.2.1.apk"
}
```

- **Las cámaras** leen `version` + `firmware_url` (`firmware/src/ota_updater.cpp`).
- **La app Android** lee `version` + `apk_url` (`UpdateChecker.kt`).

> La instalación automática de firmware viene **desactivada** (`OTA_AUTO_INSTALL=0`):
> el equipo detecta y reporta la actualización, pero no se auto-flashea. Se activa
> compilando con `-DOTA_AUTO_INSTALL=1`.

## Secrets requeridos

En **Settings → Secrets and variables → Actions** de *este* repo:

| Secret | Obligatorio | Descripción |
|---|---|---|
| `OTA_TOKEN` | ✅ | PAT con scope `repo` para leer el repo privado |
| `ANDROID_KEYSTORE_BASE64` | ✅ | Keystore de firma en base64 (ver abajo) |
| `KEYSTORE_PASSWORD` | ✅ | Contraseña del keystore |
| `KEY_PASSWORD` | ✅ | Contraseña de la clave |
| `KEY_ALIAS` | ✅ | Alias de la clave (p. ej. `camaraespia`) |
| `TG_BOT_TOKEN` | ⚪ | Token del bot de Telegram. Vacío = firmware sin Telegram |
| `WIFI_AP_PASS` | ⚪ | Clave del AP de respaldo (8+ chars). Vacío = AP abierto |

Y en el repo **privado**: `OTA_TOKEN` (el mismo PAT) para poder disparar el build.

### Generar el keystore (una sola vez)

> ⚠️ **La clave de firma no se puede cambiar nunca.** Android se niega a instalar
> una actualización firmada con otra clave; si la pierdes, los usuarios tendrían
> que desinstalar y reinstalar la app a mano. **Guarda una copia fuera de GitHub.**
>
> El workflow anterior generaba un keystore nuevo en *cada* build, así que cada
> release estaba firmada con una clave distinta y las actualizaciones fallaban.
> Por eso ahora el build **falla explícitamente** si falta el secret.

```bash
keytool -genkeypair -v \
  -keystore release.keystore \
  -alias camaraespia \
  -keyalg RSA -keysize 2048 -validity 10000 \
  -storepass 'TU_PASSWORD' -keypass 'TU_PASSWORD' \
  -dname "CN=CamaraEspia,OU=Dev,O=CamaraEspia,L=Unknown,ST=Unknown,C=MX"

# Convertir a base64 para el secret (una sola línea)
base64 -w0 release.keystore > keystore.b64      # Linux
certutil -encode release.keystore keystore.b64  # Windows (quita cabecera/pie)
```

Pega el contenido de `keystore.b64` en `ANDROID_KEYSTORE_BASE64`.

## Publicar una versión

1. Sube `FIRMWARE_VERSION` en `firmware/src/config.h` del repo privado.
2. `git push` a `main`.
3. El resto es automático: compila, firma, publica la release y actualiza
   `version.json`.

También puedes lanzarlo a mano desde **Actions → Build & Release → Run workflow**
(permite forzar una versión concreta).

### `versionCode` de Android

Se deriva de la versión (`X*10000 + Y*100 + Z`), así que crece de forma monótona
mientras subas la versión normalmente. Android **rechaza** instalar un APK con
`versionCode` menor o igual al instalado.
