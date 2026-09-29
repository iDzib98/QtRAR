# Publicar una release

Los binarios de QtRAR (`.deb` y AppImage) se publican como
[releases de GitHub](https://github.com/iDzib98/QtRAR/releases). No se suben al
repositorio: en `dist/` y en la raíz hay ficheros generados que están en
`.gitignore` a propósito.

## Qué dispara la publicación

`.github/workflows/release.yml` se ejecuta al empujar un tag con prefijo `v`:

```sh
git tag v0.1.0
git push origin v0.1.0
```

El workflow comprueba que el tag coincida con la versión de `CMakeLists.txt` y
falla si no. Un tag `v0.2.0` con la versión `0.1.0` en el código no publica nada:
preferimos un fallo a subir un `.deb` que dice una cosa y el tag otra.

## Pasos para una release

1. **Actualizar la versión** en el `project()` de `CMakeLists.txt`.
2. **Anotar los cambios** en `CHANGELOG.md` con la nueva versión y la fecha.
3. **Comprobar que todo pasa**: `ctest`, `tools/e2e-test.sh` (necesita
   `bin/rar`), `python3 tools/check-i18n.py` y `python3 tools/check-icons.py build`.
4. **Commit y subida** a `main`, y esperar a que CI esté en verde.
5. **Tag y push del tag**. El workflow hace el resto: compila el `.deb` con
   CPack, construye el AppImage, verifica que ninguno lleva `rar` ni claves,
   calcula los SHA-256 y crea la release con `gh release create --generate-notes`.

## Qué comprueba el workflow antes de publicar

- La versión del tag coincide con la de `CMakeLists.txt`.
- `ctest` pasa sobre la build de release.
- El `.deb` **no** contiene `rar` ni ningún `.key`, y **sí** contiene `unrar`.
- El AppImage, extraído, tampoco contiene `rar` ni claves, y arranca
  (`--version`).

Si cualquiera de estas comprobaciones falla, no se publica nada.

## Construir los paquetes a mano

```sh
# .deb
cmake -S . -B build-deb -DCMAKE_BUILD_TYPE=Release -DQTRAR_FETCH_RAR=OFF
cmake --build build-deb --parallel
(cd build-deb && cpack -G DEB)

# AppImage
./packaging/appimage/build-appimage.sh
```

El AppImage sale en `dist/QtRAR-<versión>-x86_64.AppImage`.

## AppImage sin SDK de Qt

`build-appimage.sh` usa, por este orden:

1. `QTRAR_QT_SDK`
2. `QT_ROOT_DIR` (lo que deja `install-qt-action` en CI)
3. El Qt 6 del sistema, montando un SDK de mentira con enlaces simbólicos

La tercera opción es la cómoda para probar en local, pero tiene una trampa: el
Qt de las distribuciones no trae `mkspecs/modules/*.pri`, que es donde los SDK
oficiales declaran qué módulos y plugins trae cada biblioteca. El script genera
esos `.pri` para los módulos que QtRAR usa. Si añades una dependencia de Qt,
revisa esa lista: si el módulo nuevo trae plugins, hay que añadirlo a
`plugin_types`.

Si el icono se ve mal, instala `librsvg2-bin` para que el script pueda convertir
el SVG a PNG (el que se usa por defecto también sirve, pero algunos gestores de
archivos renderean mejor el PNG).

## Sobre `unrar` y `rar`

El `.deb` y el AppImage llevan `unrar` porque su licencia permite redistribuirlo.
No llevan `rar`, que es la versión de prueba cuya licencia lo prohíbe. Por eso
crear y modificar archivos RAR desde QtRAR necesita que el usuario tenga `rar`
instalado por su cuenta. El detalle está en
[`licenses/THIRD-PARTY-NOTICES.md`](../licenses/THIRD-PARTY-NOTICES.md).
