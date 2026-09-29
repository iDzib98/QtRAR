# Cómo contribuir a QtRAR

Las correcciones, las ideas y las traducciones son bienvenidas. Este documento
explica cómo trabajar en el proyecto sin perder tiempo ni romper cosas.

Si prefieres el inglés, la guía corta está en el
[README en inglés](README.en.md#contributing); este fichero es la referencia
detallada.

## Antes de nada

- **Busca primero.** Puede que el fallo ya esté reportado o corregido.
- **Abre una incidencia** antes de un cambio grande. Es más barato discutir el
  enfoque en cinco minutos que descubrir en la revisión que la idea ya no cabe
  en la arquitectura.
- **Una cosa por *pull request*.** Mezclar una corrección con un cambio de estilo
  hace la revisión mucho más difícil.

## Informar de un fallo

Un informe útil lleva:

1. Versión de QtRAR (`QtRAR --version`) y de los binarios RAR
   (`rar -iver`, `unrar`).
2. Distribución, Qt (`qdpkg-query -W qt6-base`) y arquitectura.
3. Qué esperabas y qué pasó.
4. Pasos exactos para reproducirlo.
5. La salida del terminal, si la hubo. Puedes lanzar la aplicación con
   `QT_LOGGING_RULES="qt.qtrar*=true" QtRAR archivo.rar` para tener más detalle.

Los adjuntos van con datos personales o de terceros: usa archivos de prueba
sintéticos, que además sirven a los demás.

## Poner en marcha el entorno

```sh
sudo apt install build-essential cmake qt6-base-dev qt6-tools-dev-tools \
                 qt6-l10n-tools libgl1-mesa-dev
tools/fetch-rar.sh                      # desarrollo: descarga rar/unrar a bin/
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
./build/QtRAR
```

`bin/` está en `.gitignore` a propósito: los binarios de RARLAB no se pueden
versionar ni distribuir. Nunca los añadas al repositorio, ni a un paquete, ni a
un *tarball* de release.

## Pruebas

Toda corrección debería venir con una prueba, o al menos con una comprobación
manual descrita en la descripción del *pull request*.

```sh
# Núcleo (parseo de listados, servicio, diagnósticos)
ctest --test-dir build -R tst_core --output-on-failure

# Interfaz (widgets reales, sin pantalla)
ctest --test-dir build -R tst_ui --output-on-failure

# Todo
ctest --test-dir build --output-on-failure

# Traducciones: avisa de textos que se usan pero no están en el catálogo
python3 tools/check-i18n.py

# Iconos: avisa de iconos que el código pide y no están en el recurso
python3 tools/check-icons.py build

# Extremo a extremo: necesita bin/rar real, no funciona en integración continua
bash tools/e2e-test.sh
```

Con saneadores, que es como se encuentre un fallo antes de que lo encuentre
nadie:

```sh
cmake -S . -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer -g" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan -j
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir build-asan --output-on-failure
```

### Dónde va cada prueba

| Tipo | Fichero | Cuándo |
|---|---|---|
| Parseo, formato, errores | `tests/tst_core.cpp` | Cambias `src/core/`, o el texto que devuelve un binario externo. |
| Widgets, diálogos, filtros | `tests/tst_ui.cpp` | Cambias `src/ui/` o `src/model/`. |
| Comportamiento con archivos reales | `tools/e2e-test.sh` | Cambias el flujo completo o la línea de órdenes. |
| Fixtures | `tests/fixtures/*.txt` | Salidas reales de `unrar 7.23`, capturadas con `tools/capture-fixtures.sh`. Ver [`tests/fixtures/README.md`](tests/fixtures/README.md). |

Las fixtures se guardan **exactamente** como las produce RARLAB, espacios
finales incluidos: son la especificación del formato que hay que parsear. No las
"limpies" sin motivo; si cambian, se capturan otra vez y se explica por qué en
el *pull request*.

## Estilo de código

No hay una guía formal, así que estas son las convenciones que ya sigue el
código. Mantenlas y las revisiones serán más fáciles:

- **Comentarios en español**, y explican *por qué*, no *qué*. El código ya dice
  qué hace. Si un comentario parece obvio, sobra; si esconde una decisión rara
  (un `-iver`, un orden de columnas, un `QT_QPA_PLATFORM`), se queda.
- Indentación de 4 espacios, sin tabuladores. Las líneas pueden ser largas si
  partirlas dificulta la lectura de un comando o de un texto de la interfaz.
- `m_` para miembros, `k` en mayúsculas para constantes, structs en
  `PascalCase`, funciones y variables en `camelCase`.
- Qt en la cabecera: `#include <QString>` y compañía arriba del todo, después
  las cabeceras del proyecto, y la STL al final. Los `.h` del proyecto se
  incluyen con la ruta desde `src/`.
- `QStringLiteral` para textos que se comparan o se traducen, sin excepción.
- Nada de `printf`/`std::cout` en la aplicación; usa `QTextStream` sobre
  `stdout`/`stderr`.
- Las clases del núcleo no incluyen `QtWidgets`. Si necesitas un widget, es
  lógica de interfaz y va a `src/ui/`.
- La lógica que se pueda probar sin ventana, en la biblioteca: `qtrar_core` o
  `qtrar_ui`, nunca en el ejecutable.

### Cosas deliberadas: no las "arregles"

- **El orden de la barra de herramientas** replica el de WinRAR, incluidos los
  dos bloques y la etiqueta corta bajo el icono. Cambiarlo es una decisión de
  producto, no un error.
- **Los textos están en español en el código** y se traducen en los `.ts`. El
  idioma de la interfaz se elige en tiempo de ejecución; no metas literales
  traducidos a mano en el `.cpp`.
- **La lista de traducciones de `TranslationCatalog.cpp`** existe porque
  `lupdate` no ve `tr(def.textKey.c_str())`. Si añades un comando nuevo,
  actualiza esa lista o `tools/check-i18n.py` fallará.
- **Los iconos se declaran en tres sitios**: la tabla de `CommandRegistry`, las
  llamadas a `ThemeManager::icon()` y la lista `FILES` de `qt_add_resources()`.
  `tools/check-icons.py` avisa si se te olvida alguno.

## Traducciones

Añadir un idioma o corregir uno existente está en
[`docs/TRANSLATING.md`](docs/TRANSLATING.md). En corto: los `.ts` se editan,
se compilan a `.qm` con el objetivo `translations`, y `tools/check-i18n.py`
comprueba que los textos que la aplicación elige en tiempo de ejecución están
registrados.

## Proceso de revisión

Cuando llegue tu *pull request* se mira, en este orden:

1. **Que esté probado.** ¿Hay una prueba? ¿Los pasos están descritos?
2. **Coherencia con la interfaz**: atajos, textos, atajos de teclado, posición
   en los menús, comportamiento con y sin archivo abierto.
3. **Recursos**: icono nuevo en `res/icons/`, texto nuevo en los `.ts`.
4. **Ley**: ningún binario de RARLAB, ninguna clave de licencia, ningún dato
   real de terceros.
5. **Legibilidad**: ¿se entiende al leerlo? ¿El comentario explica el porqué?

No es obligatorio tener todo verde para pedir revisión, pero llegar con
`ctest` en verde ahorra discussion.

## Reglas que no se negocian

Estas no son preferencias: tienen consecuencias legales o de distribución.

1. **`rar`, `unrar` y `default.sfx` no se versionan.** `bin/` está ignorado a
   propósito. `tools/fetch-rar.sh` es una herramienta de desarrollo.
2. **Las claves de licencia no entran nunca en el repositorio.** Ni
   `rarreg.key`, ni su contenido, ni un parche para evitar la licencia.
3. **El paquete de software no puede incluir `rar`** (EULA de RARLAB, cláusula
   3.b). `unrar` sí, y es lo único que se empaqueta.
4. **No se reimplementa el algoritmo RAR.** El proyecto orquesta binarios
   oficiales; no descompila, parchea ni traduce su código.
5. **Las fixtures se capturan de verdad**, con los binarios oficiales; no se
   inventan a mano para que el test pase.

## Áreas donde tu ayuda cuenta

- Detectar ZIP cuando solo hay `unrar` y ningún 7-Zip instalado.
- Extraer un archivo parcialmente dañado, saltando lo que no se pueda.
- Integración real con CLNesh y con el gestor del portapapeles.
- Traducciones a más idiomas: los `.ts` están preparados para crecer.
- Empaquetado para otras distribuciones: RPM, Arch, Flatpak, AppImage.
- Accesibilidad: recorrido de tabulación, contraste en el tema original, lector
  de pantalla en el árbol del archivo.
- Pruebas: cada caso de `Diagnostics` y de `ListParser` que se pueda cubrir con
  una fixture nueva.

## Dudas

Si algo de este documento no se corresponde con la realidad, corrígelo en el
mismo *pull request*: es el documento más fácil de mantener del repositorio.
