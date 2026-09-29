# Arquitectura de QtRAR

Este documento explica cómo está partido el proyecto, qué hace cada clase y por
qué. Está pensado para que alguien que nunca ha visto el código sepa dónde tocar
para arreglar algo.

## Idea general

QtRAR no sabe de RAR. Es una interfaz que sabe *invocar* herramientas de línea
de comandos y *entender* lo que devuelven. Todo el trabajo real lo hacen `rar`,
`unrar` y `7z`; QtRAR se encarga de la ventana, del árbol, de los estados, de
las contraseñas y de traducir los mensajes a un idioma legible.

Consecuencia práctica: casi ningún cambio debería tocar el formato de los
archivos. Si te hace falta un dato nuevo, mira primero si algún comando de RAR
lo expone (`rar lt`, `rar vt`, `7z l -slt`…) y añade ahí el caso, no una
heurística.

## Capas

```
┌──────────────────────────────────────────────┐
│ QtRAR (src/main.cpp, src/app)                │  arranque, CLI, instancia única
├──────────────────────────────────────────────┤
│ qtrar_ui  (src/ui, src/model)                │  ventana, diálogos, temas, árbol
├──────────────────────────────────────────────┤
│ qtrar_core (src/core)                        │  procesos, parseo, servicio
└──────────────────────────────────────────────┘
                     │
              unrar / rar / 7z (QProcess)
```

Tres reglas mantienen el dibujo:

1. **`qtrar_core` no incluye `QtWidgets`.** Solo `QtCore` y `QtGui`. Ahí viven
   el parseo de los listados, la construcción de la línea de órdenes, la
   detección de errores y el diagnóstico. Se puede probar en consola.
2. **`qtrar_ui` es una biblioteca, no parte del ejecutable.** El filtro de
   búsqueda, el modelo del árbol y la validación de los diálogos son lógica de
   verdad y merecen una prueba sin montar una ventana. `tst_ui` enlaza contra
   esta biblioteca.
3. **El ejecutable solo arranca.** `main.cpp` crea la `Application`, lee los
   argumentos, decide si hace falta una ventana y se la pasa. Cualquier cosa
   que se pueda probar, bájala de ahí.

## `src/core`

| Fichero | Responsabilidad |
|---|---|
| `ProcessRunner` | Ejecuta un binario, captura stdout/stderr y traduce el código de salida. Es síncrono y sin *timeout*: un `rar` colgado deja la interfaz esperando. Primera piedra angular para el futuro asíncrono. |
| `BinaryLocator` | Busca `unrar`, `rar` y `7z` (o `7za`, `7zz`, `7zr`). Orden: variable de entorno (`QTRAR_RAR`, `QTRAR_UNRAR`, `QTRAR_7Z`), ruta configurada en `tools/rarPath`, carpetas junto al ejecutable, `bin/` del árbol de fuentes y por último el `PATH`. También deduce la versión con la primera línea de salida. |
| `ListParser` | Convierte la salida de `unrar lt` / `rar lb` / `7z l -slt` en filas `ArchiveEntry`. Cada formato tiene su propio lector, y `tests/fixtures/` contiene salidas reales para todos ellos. |
| `ArchiveService` | El corazón: una operación por método (`list`, `test`, `extract`, `create`, `add`, `rename`, `remove`, `comment`, `protect`, `repair`, `volumes`, `sfx`). Decide qué binario usar según la extensión, pide contraseña cuando hace falta y devuelve un `OperationResult` con un código de diagnóstico. |
| `Diagnostics` | Traduce el código de salida de RAR y el texto de error a algo que se pueda mostrar y traducir. Es el punto donde se decide si un "no se encontró el volumen" es un error o un aviso. |
| `HFormat` | Formatea tamaños, ratios, fechas y attribute flags en el estilo de WinRAR. |
| `Types` | `ArchiveEntry`, `ArchiveFormat`, `CreateOptions`, `OperationResult`, `DiagnosticCode` y compañía. |
| `LicenseProbe` | Ejecuta `rar -iver` para saber si el binario está en modo de evaluación. No lee ni escribe claves. |
| `TranslationCatalog` | Lista de textos que se traducen en tiempo de ejecución y que `lupdate` no puede detectar. La vigila `tools/check-i18n.py`. |

### Dónde tocar para cada cosa

| Quiero… | Toco… |
|---|---|
| Reconocer un error nuevo de RAR | `Diagnostics` + un caso en `tst_core` |
| Entender un formato de listado nuevo | `ListParser` + una fixture nueva |
| Añadir una operación (p. ej. convertir a ZIP) | `ArchiveService` + `CommandRegistry` + el diálogo que la llame |
| Buscar otro binario en otro sitio | `BinaryLocator::locateOne` |
| Cambiar cómo se ejecuta un proceso | `ProcessRunner` (y luego adaptar a `ArchiveService`) |

## `src/ui` y `src/model`

| Fichero | Responsabilidad |
|---|---|
| `MainWindow` | Ensambla la ventana: menús, barra, paneles, diálogo de extracción, progreso, mensajes. Es grande a propósito, porque todo lo visible está aquí y dispersarlo costaría más. |
| `CommandRegistry` | Una tabla con todas las acciones: texto, icono, atajo, en qué menú, en qué grupo de la barra y si se habilita con o sin archivo. Menús y barra se generan a partir de ella, así que añadir un comando es una fila. |
| `ThemeManager` | Carga el QSS del tema original, alterna con el del sistema, crea `QIcon` desde los SVG empaquetados y sigue el cambio de esquema de color del escritorio. |
| `ExtractDialog` | El diálogo *Extraer en*: pestañas General/Avanzado/Opciones, árbol de destino, modo de sobrescritura, y validación de los campos antes de aceptar. |
| `CreateDialog` | El diálogo de creación: formato, nivel, sólido, cifrado, volúmenes y autoextraíble. |
| `ToolbarDialog` | *Personalizar barra de herramientas…*: qué botones se ven y en qué orden. |
| `OptionsDialog` | Preferencias, que son directamente las claves de `QSettings` de la tabla del README. |
| `RarSetupDialog` | Localiza el binario `rar` oficial y guarda `tools/rarPath`. |
| `PasswordDialog` | Pide la clave y la mantiene en memoria durante la sesión. |
| `ProgressDialog` | Muestra la salida de progreso de RAR, que viene como líneas con `\r` y porcentajes. |
| `ArchiveTreeModel` | El modelo del panel derecho. Convierte la lista plana de `ListParser` en carpetas virtuales, inserta la fila `..`, y decide el icono según el tipo (carpeta, imagen, código, medio, archivo) y el estado (bloqueado, cifrado). |
| `FindProxyModel` | Filtro recursivo del buscador, con comodines. |
| `StatusPanel` | La barra de abajo: nombre, formato, tamaño, cantidad de ficheros, tiempo. |
| `AddressBar` | Barra de direcciones con el historial de navegación. |
| `VirusScan` | Escanea un archivo recién extraído con el antivirus del sistema, si lo hay. |

## `src/app`

- `Application` hereda de `QApplication`: fija nombre, versión, estilo Fusion,
  carga las traducciones (las de Qt y las propias) y **parsea la línea de
  órdenes antes de crear ninguna ventana**. `--help` y `--version` salen sin
  arrancar la interfaz.
- Instancia única mediante `QLocalServer`: el segundo proceso pasa la ruta del
  archivo al primero y termina.
- `HeadlessProbe` es una sonda sin ventana que ejecuta exactamente el mismo
  código que la ventana y vuelca el resultado en texto
  (`--dump`, y las variables `QTRAR_*` de `tools/e2e-test.sh`). Existe para que
  las pruebas de extremo a extremo no dependan del sincronismo de los widgets.

## Recursos

- `res/icons/*.svg` se compilan en el binario con `qt_add_resources()`, así que
  la aplicación funciona sin ficheros sueltos al lado.
- `res/themes/original.qss` es el tema clásico; el sistema provee el otro.
- `res/i18n/*.ts` son los catálogos. El objetivo `translations` los compila a
  `.qm` y los copia junto al ejecutable, en el árbol de fuentes y en la
  instalación.

## Flujo de una operación

Ejemplo: *Extraer en…* con un archivo cifrado.

1. `MainWindow` valida el diálogo y llama a `ArchiveService::extract()`.
2. `ArchiveService` decide el formato por extensión, localiza el binario con
   `BinaryLocator` y lanza `unrar x -p… -o+ …` con `ProcessRunner`.
3. Si RAR responde con el código de "se requiere contraseña",
   `Diagnostics` lo clasifica y `ArchiveService` devuelve
   `OperationResult{needsPassword}` en vez de un error.
4. `MainWindow` pide la clave con `PasswordDialog`, guarda el resultado para las
   siguientes llamadas de la sesión y repite el paso 2.
5. `ProgressDialog` va leyendo la salida de progreso mientras dura el proceso.
6. Al terminar, `MainWindow` abre la carpeta de destino o refresca el panel,
   según las opciones del diálogo.

Lo mismo con un matiz en el paso 2 para ZIP: si no hay 7-Zip, `ArchiveService`
avisa en lugar de fingir que sabe leer el formato.

## Pruebas y por qué están separadas

`src/core` y `src/ui` se prueban con `tst_core` y `tst_ui`. La separación no es
estética: el núcleo tiene la lógica difícil (parsear una salida de RAR y decidir
qué hacer con un código de error 3), y la interfaz tiene la tediosa (un diálogo
que no debe aceptar un destino vacío). Cada uno se prueba con la herramienta
adecuada, y `tst_ui` puede correr con `QT_QPA_PLATFORM=offscreen`, sin servidor
gráfico.

`tools/e2e-test.sh` existe para lo que ninguna de las dos cubre: que el
comando que se construye sea el que RAR entiende. Usa la sonda sin ventana, así
que es rápida y determinista, y necesita los binarios reales.

## Deuda técnica conocida

Está en la lista de abajo del README, pero aquí con el porqué:

- **`ProcessRunner` es síncrono.** Es la limitación estructural más importante:
  mientras un RAR grande se extrae, el proceso principal está bloqueado y no se
  puede cancelar. La ruta natural es moverlo a un hilo con señales de progreso
  y cancelación, empezando por el *timeout* de `BinaryLocator::versionOf`.
- **`encryptFileNames` guarda prefijos absolutos.** Al cifrar nombres sobre un
  archivo abierto desde una ruta absoluta, el prefijo se persiste tal cual, así
  que un archivo movido de sitio deja de poder renombrarse. Es un caso raro y no
  arreglado.
- **Opciones de creación incompletas.** `CreateOptions` tiene más campos de los
  que el diálogo expone (volúmenes, filtros, recovery record, tiempo…).
- **Comentarios y multivolumen.** `Diagnostics` no distingue todavía "contraseña
  incorrecta" de "falta un volumen": el mensaje es el mismo para los dos.

Cualquier *pull request* que ataque uno de estos puntos es bienvenido; si
tocaras `ProcessRunner`, avisa antes por el impacto en las llamadas.
