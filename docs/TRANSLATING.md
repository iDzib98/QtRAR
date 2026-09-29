# Traducciones

QtRAR se escribe en español y se traduce con las herramientas de Qt
(`lupdate` y `lrelease`). Los archivos de la interfaz están en español, así que
**el idioma de origen es el español**: las traducciones nuevas se hacen desde
`qtrar_es.ts`, y `qtrar_en.ts` es la de referencia para quien lee inglés.

## Dónde están las cosas

| Elemento | Ruta |
|---|---|
| Catálogos | `res/i18n/qtrar_es.ts`, `res/i18n/qtrar_en.ts` |
| Compilados `.qm` | Se generan en la compilación y se copian a `<build>/i18n/` |
| Instalados | `share/qtrar/i18n/qtrar_XX.qm` |
| Lista de idiomas | `src/ui/dialogs/OptionsDialog.cpp` (`m_languageCombo`) |
| Textos que `lupdate` no ve | `src/core/TranslationCatalog.cpp` |
| Comprobación | `tools/check-i18n.py` |

En ejecución, los `.qm` se buscan en dos sitios, en este orden:

1. `<directorio del ejecutable>/i18n` — caso normal en una instalación y en el
   árbol de fuentes.
2. `<directorio del ejecutable>/../share/qtrar/i18n` — por si se instala a mano.

El idioma se elige en *Opciones → Idioma*, o con `QtRAR -l en`. Si está vacío,
se usa el del sistema (`es_ES` → `es`).

## Traducir o corregir un idioma que ya existe

1. Edita el `.ts` con `pylupdate` de PyQt o con Qt Linguist:

   ```sh
   # actualizar el catálogo con los textos nuevos del código
   cmake --build build --target update_translations

   # abrirlo en Qt Linguist
   linguist res/i18n/qtrar_en.ts
   ```

2. Traduce solo los `<translation>` que estén vacíos. Los `type="vanished"`
   aparecen tachados: son textos que ya no existen en el código y no se compilan.

3. Compila y prueba:

   ```sh
   cmake --build build -j
   QT_QPA_PLATFORM=offscreen ./build/QtRAR -l en
   python3 tools/check-i18n.py
   ```

Un par de reglas que ahorra discusión en la revisión:

- **No traduzcas los códigos de error de RAR** ni los nombres de las opciones de
  la línea de órdenes (`-o+`, `-ep1`, `Solid`, `Volume`). Son texto para
  diagnosticar, y traducirlos impide buscar el mensaje original.
- **Mantén los puntos suspensivos y los atajos con `&`** como están: forman parte
  del texto de la interfaz, no de la traducción.
- Si un texto lleva `<code>`, `<b>` o `<br>`, respétalos: son etiquetas de Qt y
  tienen que sobrevivir a la traducción.

## Añadir un idioma nuevo

Por ejemplo, el alemán (`de`):

1. Copia el catálogo de referencia y renómbralo:

   ```sh
   cp res/i18n/qtrar_en.ts res/i18n/qtrar_de.ts
   ```

2. Edítalo: cambia el `language` del `<TS>` a `de_DE` y traduce los textos.

3. Añádelo a la lista de la compilación en `CMakeLists.txt`:

   ```cmake
   set(QTRAR_TS_FILES
       res/i18n/qtrar_es.ts
       res/i18n/qtrar_en.ts
       res/i18n/qtrar_de.ts
   )
   ```

4. Añádelo al desplegable de *Opciones → Idioma*, junto a los otros, con el
   nombre del idioma **traducido a sí mismo** (así se ve en la lista):

   ```cpp
   m_languageCombo->addItem(QStringLiteral("Deutsch"), QStringLiteral("de"));
   ```

5. Compila, prueba con `QtRAR -l de` y abre el *pull request* indicando que es
   un idioma nuevo.

Las traducciones de Qt (los botones de `QDialogButtonBox`, por ejemplo) las carga
la propia aplicación desde las de Qt, con el idioma completo y con el código de
dos letras:

```
qtrar_de → qtbase_de, qtbase_de_DE → qtrar_de_DE
```

## El catálogo de `TranslationCatalog.cpp`

`CommandRegistry` guarda el texto de cada acción en una tabla y lo traduce con
`tr(def.textKey.c_str())`, y las etiquetas cortas de la barra usan
`QCoreApplication::translate("qtrar::CommandRegistry", "...")`. Ninguna de las
dos formas es un `tr("literal")` que `lupdate` pueda seguir, así que esos textos
se registran a mano con `QT_TRANSLATE_NOOP` en `src/core/TranslationCatalog.cpp`.

Si añades un comando nuevo a `CommandRegistry` y no lo añades ahí, el menú
saldrá en inglés (o en el idioma de origen) sin que el compilador se queje. Por
eso `tools/check-i18n.py` compara las dos listas:

```sh
python3 tools/check-i18n.py
```

```
FALTA en el catálogo: Añadir
```

## Comprobar que nada se ha roto

```sh
python3 tools/check-i18n.py
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Las tres cosas juntas: el catálogo al día, los `.qm` recompilados y las
traducciones de Qt cargadas sin quejarse de que falte un idioma (eso no es un
error, se ignora a propósito).
