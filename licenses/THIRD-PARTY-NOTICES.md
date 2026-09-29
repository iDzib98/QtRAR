# Archivos de licencia de terceros

Aquí no está la licencia de QtRAR. La licencia del proyecto es
[GPL-3.0-or-later](../LICENSE), en la raíz del repositorio.

Estos dos ficheros son copias de los textos que vienen dentro del tarball
oficial de RARLAB (`rarlinux-x64-7.23.tar.gz`), que descarga
`tools/fetch-rar.sh`:

| Fichero | Qué es |
|---|---|
| `license.txt` | El *End User License Agreement* de RAR (WinRAR/RAR) de win.rar GmbH. |
| `acknow.txt` | Los créditos y licencias de los componentes del código de RAR, incluidos los de terceros. |

Se versionan y se instalan en el paquete (`share/doc/qtrar/`) por dos razones:

1. **Obligación de la licencia.** La EULA de RARLAB exige que sus textos de
   licencia y reconocimiento acompañen a sus componentes. El paquete incluye
   `unrar`, así que tiene que incluir también esto.
2. **Que no haya que volver a la web de RARLAB** para saber qué se puede hacer
   con lo que se descarga.

## Lo que permite la EULA, en resumen

- `unrar` se puede redistribuir por separado. Es lo único que QtRAR empaqueta
  (opción CMake `QTRAR_BUNDLE_UNRAR`).
- `rar` es la versión de prueba y **no** puede distribuirse dentro de otro
  paquete de software (cláusula 3.b). Por eso `bin/` está en `.gitignore` y el
  paquete Debian no lo contiene.
- No se puede modificar la EULA, ni incluir cracks, llaves ni generadores de
  llaves, ni Reverse engineer para reconstruir el algoritmo RAR.

El resumen completo, con la redacción original, está en
[`README.md`](../README.md#binarios-de-rar-y-licencias).

## Otros componentes

- **Qt** y el estilo Fusion: LGPL v3 y GPL v3, respectivamente.
- **7-Zip**, si se usa para el soporte de ZIP: LGPL v2.1 con la excepción de
  7-Zip. QtRAR no lo incluye, solo lo invoca si está instalado.
