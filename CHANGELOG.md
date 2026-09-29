# Changelog

Formato basado en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/), y
versionado según [SemVer](https://semver.org/lang/es/).

## [No publicado]

Nada todavía.

## [0.1.0] — 2026-09-28

Primera versión pública. Alfa: usable a diario, pero incompleta.

### Añadido

- Núcleo RAR y ZIP sobre los binarios oficiales de RARLAB y 7-Zip: listar,
  probar, extraer, crear, modificar, renombrar, borrar, comentar, proteger,
  reparar, multivolume y autoextraíbles.
- Interfaz estilo WinRAR: explorador de archivos y panel de archivo, barra de
  direcciones con historial, favoritos, atrás/adelante y fila `..`.
- Barra de herramientas con el orden y las etiquetas de WinRAR, y diálogo para
  personalizarla.
- Diálogo *Extraer en…* con pestañas General, Avanzado y Opciones, árbol de
  carpetas de destino y modos de actualización y sobrescritura.
- Extracción de un miembro concreto con doble clic, conservando el archivo
  temporal mientras la aplicación asociada lo usa.
- Diálogo de creación con nivel de compresión, sólidos, contraseña, cifrado de
  nombres, volúmenes y autoextraíbles.
- Asistente de creación paso a paso, buscador con comodines y diálogo de
  información del archivo.
- Panel de comentario que aparece al abrir el archivo y se actualiza al guardar.
- Temas `original` y `system`, con iconos claros y oscuros y seguimiento del
  esquema de color del escritorio.
- Localización de los binarios de RAR, diálogo para elegir la ruta de `rar` y
  estado de la licencia de RAR (evaluación de 40 días o registrada).
- Integración de escritorio: acciones contextuales en Dolphin y Nautilus, tipos
  MIME, y opciones de línea de órdenes (`--test`, `--extract-here`,
  `--extract-to`, `--extract-to-dialog`, `--add-to-archive`, `--dump`).
- Traducciones al español y al inglés, incluidas las de Qt.
- Paquete Debian con CPack.
- Pruebas: `tst_core`, `tst_ui`, un script de extremo a extremo y
  comprobaciones de traducciones e iconos.

### Pendiente

- `ProcessRunner` es síncrono: las operaciones largas bloquean la interfaz y no
  se pueden cancelar.
- El diálogo de creación no expone todas las opciones de `CreateOptions`.
- Al cifrar nombres sobre un archivo abierto desde una ruta absoluta se guarda
  un prefijo absoluto, y el archivo deja de poder renombrarse si se mueve.
- `Diagnostics` no distingue entre contraseña incorrecta y volumen ausente.

[No publicado]: https://github.com/iDzib98/QtRAR/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/iDzib98/QtRAR/releases/tag/v0.1.0
