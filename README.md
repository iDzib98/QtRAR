# QtRAR

[Español](README.md) · [English](README.en.md)

Gestor de archivos RAR y ZIP para Linux con la interfaz de WinRAR, escrito en
C++20 y Qt 6. QtRAR no implementa el formato RAR: se apoya en los binarios
oficiales de RARLAB (`unrar` y `rar`) y, para ZIP, en 7-Zip.

![Licencia: GPL-3.0](https://img.shields.io/badge/licencia-GPLv3-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.4%2B-41cd52.svg)
![C++](https://img.shields.io/badge/C%2B%2B-20-f34b7d.svg)

> **Estado: 0.1.0 (alfa).** La aplicación es usable a diario para extraer, probar
> y navegar, pero todavía faltan cosas y hay Roads abiertos. Si algo falla,
> [abre una incidencia](CONTRIBUTING.md#informar-de-un-fallo) con la salida
> exacta del terminal: eso es lo que más ayuda.

## Por qué existe

En Linux, WinRAR no está disponible como aplicación nativa y las opciones que
existen o son de terminal (`unar`, `file-roller`) o tienen una interfaz muy
distinta. QtRAR quiere ser la respuesta obvia para quien llega desde Windows:
la misma disposición, los mismos botones, los mismos atajos, pero compilada
nativamente y sin depender de Wine.

## Qué hace

- **Navegación al estilo WinRAR**: explorador de archivos a la izquierda y
  contenido del archivo a la derecha, con barra de direcciones, atrás/adelante,
  historial, favoritos, y fila `..` para subir de nivel. Al abrir un archivo, el
  panel izquierdo pasa a mostrar el árbol interno del RAR o ZIP.
- **Barra de herramientas idéntica a la de WinRAR** (32 px, etiqueta corta bajo
  el icono), con el bloque derecho de herramientas que cambia según haya un
  archivo abierto o no. Se puede personalizar desde *Personalizar barra de
  herramientas…*.
- **Extracción**: todo, la carpeta del archivo, la selección, o un miembro
  concreto con doble clic. Diálogo con pestañas *General* / *Avanzado* /
  *Opciones*, árbol de carpetas de destino, modos de actualización y
  sobrescritura.
- **Creación** de archivos RAR y ZIP: nivel de compresión, sólidos, cifrado con
  contraseña, **cifrado de nombres**, volúmenes multivolume y autoextraíbles.
- **Mantenimiento**: comprobar integridad, renombrar, borrar, proteger, reparar
  cabeceras recuperables, escribir y leer el comentario del archivo.
- **Contraseñas**: se piden cuando hacen falta y se recuerdan durante la sesión
  (opción de preguntarla siempre).
- **Asistente** de creación paso a paso y **buscador** de archivos dentro del
  archivo con comodines (`*`, `?`).
- **Temas** `original` (el clásico de WinRAR) y `system`, con iconos claros y
  oscuros, siguiendo el cambio de color del escritorio.
- **Integración de escritorio**: acciones *Extraer aquí*, *Extraer en…*,
  *Comprobar* y *Añadir a un archivo…* en Dolphin y Nautilus, registro de tipos
  MIME, y línea de órdenes para scripts.
- **Internacionalización** en español e inglés, con las traducciones de Qt
  cargadas también.

## Requisitos

| | |
|---|---|
| Qt | 6.4 o superior (Core, Gui, Widgets, Network, LinguistTools, Test) |
| CMake | 3.21 o superior |
| Compilador | C++20 (GCC 12+, Clang 15+, o el que traiga tu distribución) |
| RAR | `unrar` para listar, probar y extraer; `rar` para crear y modificar |
| ZIP | 7-Zip (`7z`, o `7za` / `7zz` / `7zr` como alternativa) |

En Debian/Ubuntu:

```sh
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev-tools \
                 qt6-l10n-tools libgl1-mesa-dev
```

## Instalación

### Paquete Debian

Descarga el `.deb` de la versión publicada y:

```sh
sudo apt install ./qtrar_0.1.0_amd64.deb
```

El paquete incluye QtRAR, `unrar` (el único componente de RAR que su licencia
permite redistribuir), las traducciones y las integraciones con Dolphin y
Nautilus. **No** incluye `rar` ni claves de licencia; ver
[Binarios de RAR y licencias](#binarios-de-rar-y-licencias).

Para construir el paquete tú mismo, ver [`packaging/README-deb.md`](packaging/README-deb.md).

### Desde las fuentes

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/QtRAR
```

La primera configuración intenta descargar los binarios oficiales de RAR a
`bin/` con `tools/fetch-rar.sh` (opción `QTRAR_FETCH_RAR`, activa por defecto).
Si prefieres no descargarlos:

```sh
cmake -S . -B build -DQTRAR_FETCH_RAR=OFF
```

Para instalar en tu usuario, sin permisos de administrador:

```sh
cmake --install build --prefix ~/.local
```

Después, refresca la base de datos de aplicaciones para que aparezcan los
tipos MIME y los menús contextuales:

```sh
update-desktop-database
```

Las opciones de CMake que importan:

| Opción | Por defecto | Qué hace |
|---|---|---|
| `QTRAR_FETCH_RAR` | `ON` | Descarga `rar`/`unrar` a `bin/` si no están. Solo para desarrollo: `bin/` no se versiona. |
| `QTRAR_BUNDLE_UNRAR` | `OFF` | Instala `unrar` junto a QtRAR. Es la redistribución que la EULA de RARLAB permite (cláusula 3.a). |
| `BUILD_TESTING` | `ON` | Compila `tst_core` y `tst_ui` y los registra en CTest. |

Las integraciones de explorador de archivos se describen en
[`packaging/README-context-menu.md`](packaging/README-context-menu.md).

## Uso desde la terminal

QtRAR acepta archivos y acciones, para poder llamarlo desde scripts o desde los
menús contextuales:

```sh
QtRAR archivo.rar                    # abre el archivo
QtRAR --test archivo.rar             # comprueba la integridad y termina
QtRAR --extract-here archivo.rar     # extrae junto al archivo y termina
QtRAR --extract-to /destino a.rar    # extrae en la carpeta indicada y termina
QtRAR --extract-to-dialog archivo.rar  # abre el diálogo de extracción
QtRAR --add-to-archive ficheros...   # abre el diálogo de creación
QtRAR -l en --theme system           # idioma y tema
QtRAR --help
```

La aplicación es de instancia única: si ya hay una ventana abierta, el segundo
lanzamiento le pasa el archivo y sale (se puede desactivar en *Opciones*).

### Dónde se guardan las preferencias

En `~/.config/QtRAR/QtRAR.conf` (formato INI de `QSettings`). Claves usadas:

| Clave | Significado |
|---|---|
| `ui/language` | Idioma de la interfaz (`es`, `en`). |
| `ui/theme` | `0` = tema original, `1` = el del sistema. |
| `ui/icons` | Tema de iconos. |
| `app/singleInstance` | Instancia única. |
| `paths/extractTo` | Carpeta de extracción por defecto; si está vacía se usa la del archivo. |
| `files/confirmOverwrite` | Preguntar antes de sobrescribir. |
| `files/createVolumes` | Crear volúmenes por defecto. |
| `files/compressionLevel` | Nivel de compresión por defecto. |
| `window/rememberGeometry` | Recordar tamaño y posición de la ventana. |
| `tools/rarPath` | Ruta al binario `rar` que puedes elegir desde *Configurar binario RAR…*. |

## Binarios de RAR y licencias

Esta es la parte que conviene leer antes de redistribuir QtRAR.

QtRAR **no** incluye el algoritmo de RAR ni los binarios de RARLAB en el
repositorio. El proyecto necesita un `rar` y un `unrar` en el sistema, o
descargados en `bin/` para desarrollo:

```sh
tools/fetch-rar.sh          # descarga rarlinux-x64-7.23 a bin/ y verifica el SHA-256
```

Según la licencia de RARLAB (`licenses/license.txt`, cláusula 3):

- `rar` es una versión de prueba y **no puede distribuirse dentro de otro
  paquete de software**. Por eso `bin/` está en `.gitignore` y el paquete Debian
  no lo incluye. Cada usuario lo instala por su cuenta, o usa 7-Zip, que abre y
  extrae RAR pero no lo crea.
- `unrar` sí puede redistribuirse por separado; es lo único que QtRAR empaqueta
  (`QTRAR_BUNDLE_UNRAR=ON`).
- Las claves de licencia (`rarreg.key`) nunca deben entrar en el repositorio.
  QtRAR no lee, copia ni escribe la clave: solo indica si el binario `rar` la
  encuentra y si está en modo de evaluación de 40 días. La validación es cosa de
  RAR.

El código de QtRAR es software libre: **GPL-3.0-or-later** (ver
[`LICENSE`](LICENSE)). `licenses/license.txt` y `licenses/acknow.txt` son
copias de los textos de RARLAB y no son la licencia de este proyecto; para el
detalle de los componentes de terceros, ver
[`licenses/THIRD-PARTY-NOTICES.md`](licenses/THIRD-PARTY-NOTICES.md).

## Desarrollo

Resumen rápido; los detalles están en [`CONTRIBUTING.md`](CONTRIBUTING.md).

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j

ctest --test-dir build --output-on-failure     # tests de núcleo e interfaz
python3 tools/check-i18n.py                    # textos en el catálogo de traducción
python3 tools/check-icons.py build             # iconos pedidos por el código
bash tools/e2e-test.sh                         # extremo a extremo (necesita bin/rar)
```

Las pruebas de interfaz corren con `QT_QPA_PLATFORM=offscreen`, así que no
necesitan pantalla. Las de extremo a extremo sí necesitan los binarios reales de
RAR y no se pueden ejecutar en integración continua sin distribuirlos.

## Estructura del proyecto

```
src/core/     Núcleo sin widgets: ejecución de procesos, parseo de listados,
              servicio de archivo, localización de binarios, diagnóstico.
src/ui/       Ventana principal, barra, diálogos, temas, búsqueda.
src/model/    Modelo de árbol del archivo (carpetas virtuales, iconos, `..`).
src/app/      Arranque de la aplicación, sonda sin interfaz, CLI.
res/          Iconos SVG, hoja de estilo del tema original, traducciones .ts.
packaging/    Archivos .desktop, scripts de Nautilus, receta del paquete DEB.
tests/        tst_core, tst_ui, capturas de salida real de unrar.
tools/        Descarga de RAR, comprobaciones de i18n e iconos, E2E.
docs/         Documentación paraarchical y de arquitectura.
```

Un detalle de diseño que conviene conocer antes de tocar nada: la lógica está
dividida en dos bibliotecas, `qtrar_core` (sin `QtWidgets`) y `qtrar_ui`, y el
ejecutable solo arranca. Así el parseo de listados y los filtros se prueban sin
montar una ventana. Ver [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md).

## Documentación

| Documento | Contenido |
|---|---|
| [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) | Capas, clases, flujo de una operación y dónde tocar para cada cosa. |
| [`docs/TRANSLATING.md`](docs/TRANSLATING.md) | Cómo añadir o corregir una traducción. |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | Cómo contribuir, estilo de código, pruebas y proceso de revisión. |
| [`CHANGELOG.md`](CHANGELOG.md) | Historial de versiones. |
| [`packaging/README-deb.md`](packaging/README-deb.md) | Construir el paquete Debian. |
| [`packaging/README-context-menu.md`](packaging/README-context-menu.md) | Menús contextuales y tipos MIME. |
| [`tests/fixtures/README.md`](tests/fixtures/README.md) | Cómo se capturan y para qué sirven las fixtures. |
| [`licenses/THIRD-PARTY-NOTICES.md`](licenses/THIRD-PARTY-NOTICES.md) | Licencias de terceros (RARLAB, Qt, 7-Zip) y qué se puede distribuir. |

## Cómo contribuir

Las correcciones, las ideas y las traducciones son bienvenidas. Empieza leyendo
[`CONTRIBUTING.md`](CONTRIBUTING.md), pero en corto:

1. Abre una incidencia antes de escribir un cambio grande, para no perder el
   trabajo discutiendo el enfoque.
2. Trabaja sobre una rama desde `main` y haz *pull request* describiendo qué
   cambia y por qué.
3. Añade o ajusta pruebas: `tst_core` para el núcleo, `tst_ui` para la interfaz,
   `tools/e2e-test.sh` para el comportamiento con archivos reales.
4. Mantén el orden de la barra de herramientas y los textos: son deliberados.

Áreas donde la ayuda aporta más ahora mismo:

- Detectar ZIP cuando solo hay `unrar` y ningún 7-Zip instalado.
- Descomprimir un archivo parcialmente dañado durante la extracción.
- Análisis sin desensamblaje, integración real con CLNesh, portapapeles.
- Traducciones a más idiomas.
- Empaquetado para otras distribuciones (RPM, Arch, Flatpak).

## Licencia

QtRAR está bajo **GPL-3.0-or-later**; ver [`LICENSE`](LICENSE) para el texto
completo. Los binarios de RARLAB que se usan en tiempo de ejecución **no** se
distribuyen con este proyecto y tienen su propia licencia.

## Agradecimientos

- A [RARLAB](https://www.rarlab.com/), por `rar` y `unrar`, y por su licencia,
  que permite usar y redistribuir `unrar` de forma compatible con un proyecto
  libre.
- Al proyecto [Qt](https://www.qt.io/) y a quien mantiene el estilo Fusion, que
  es la base del aspecto clásico de la interfaz.
- A [7-Zip](https://www.7-zip.org/), por cubrir ZIP.
- A Intel, por el código de *slicing-by-8* citado en `licenses/acknow.txt`.
- A quien envíe una corrección, una traducción o una idea. Empieza la lista tú.
