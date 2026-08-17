# CamaraEspia-OTA

Repositorio público para OTA updates y releases de CamaraEspia.

## ¿Cómo funciona?

1. El repo privado [CamaraEspia](https://github.com/oscgtz2107-ui/CamaraEspia) dispara un `repository_dispatch` cuando hay cambios en firmware/app
2. Este repo compila el firmware y publica una release
3. Las placas ESP32 consultan `https://oscgtz2107-ui.github.io/CamaraEspia-OTA/version.json` para detectar actualizaciones

## Secrets requeridos

En **Settings → Secrets → Actions** del repo público:

| Secret | Descripción |
|--------|-------------|
| `OTA_TOKEN` | Personal Access Token con scope `repo` que pueda acceder al repo privado |

## Endpoints

- `https://oscgtz2107-ui.github.io/CamaraEspia-OTA/version.json` — Versión actual
- `https://github.com/oscgtz2107-ui/CamaraEspia-OTA/releases/latest` — Última release
