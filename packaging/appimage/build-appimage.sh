#!/usr/bin/env bash
#
# Construye el AppImage de QtRAR.
#
# Un AppImage lleva dentro la aplicacion, sus bibliotecas y sus plugins, y se
# ejecuta sin instalar nada. Se genera a partir de un AppDir, que es un arbol
# de directorios con esta forma minima:
#
#   AppDir/
#     AppRun            arranque (packaging/appimage/AppRun)
#     qtrar.desktop     lo lee appimagetool al construir la imagen
#     qtrar.png         icono; el SVG tambien vale, pero PNG es lo habitual
#     usr/              aplicacion, bibliotecas de Qt, datos y traducciones
#
# Requisitos: bash, cmake, un compilador C++20, curl, python3, patchelf y Qt 6.
# Las tres herramientas de terceros (appimagetool, patchelf y el desplegador
# de Qt) se descargan solas al directorio de cache, salvo que ya esten
# disponibles en el sistema.
#
# Uso:
#   packaging/appimage/build-appimage.sh [directorio-de-salida]
#
# Variables de entorno utiles:
#   QTRAR_QT_SDK          Instalacion de Qt 6 tipo /opt/Qt/6.8.2/gcc_64.
#                         Si no se indica, se busca $QT_ROOT_DIR y, si tampoco,
#                         se construye un arbol de enlaces con el Qt del
#                         sistema (/usr), que es lo habitual en Debian.
#   QTRAR_BUILD_DIR       Directorio de compilacion (build-appimage).
#   QTRAR_TOOLS_DIR       Cache de las herramientas descargadas.
#   QTRAR_APPIMAGETOOL    Usar una copia local de appimagetool.
set -euo pipefail

RAIZ="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="${QTRAR_BUILD_DIR:-$RAIZ/build-appimage}"
SALIDA="${1:-$RAIZ/dist}"
TOOLS="${QTRAR_TOOLS_DIR:-$RAIZ/build-appimage-tools}"
APPDIR="$BUILD/AppDir"

# El nombre del artefacto sale de la version de CMakeLists.txt, que es la que
# va tambien en el paquete .deb y en `--version`: si se mezclaran, el usuario
# no sabria cual ha descargado.
VERSION="$(awk '/^project\(QtRAR/ { en_proyecto = 1 }
                  en_proyecto && /VERSION/ { gsub(/[^0-9.]/, ""); print; exit }' \
           "$RAIZ/CMakeLists.txt")"
[ -n "$VERSION" ] || { echo "error: no se encontro VERSION en CMakeLists.txt" >&2; exit 2; }

APPNAME="QtRAR"
APPIMAGE="$SALIDA/QtRAR-$VERSION-x86_64.AppImage"

descargar() {   # descargar <url> <destino> <ejecutable-o-no>
    local url="$1" destino="$2" modo="${3:-}"
    if [ -f "$destino" ] && [ -n "$modo" ] && [ -x "$destino" ]; then
        echo "==> ya esta: $(basename "$destino")"
        return
    fi
    echo "==> descargando $(basename "$destino")"
    curl -fsSL --retry 3 --max-time 300 -o "$destino.tmp" "$url"
    mv "$destino.tmp" "$destino"
    # Ojo: `[ -n ... ] && chmod ...` con `set -e` aborta el script entero si la
    # condicion es falsa, que es justo lo que pasa cuando no hay que dar permisos.
    if [ -n "$modo" ]; then
        chmod +x "$destino"
    fi
    return 0
}

# --- 1. Herramientas --------------------------------------------------------
mkdir -p "$TOOLS"

if [ -z "${QTRAR_APPIMAGETOOL:-}" ]; then
    if command -v appimagetool >/dev/null 2>&1; then
        QTRAR_APPIMAGETOOL="$(command -v appimagetool)"
    else
        descargar "https://github.com/probonopd/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" \
                 "$TOOLS/appimagetool" x
        QTRAR_APPIMAGETOOL="$TOOLS/appimagetool"
    fi
fi

if ! command -v patchelf >/dev/null 2>&1; then
    descargar "https://github.com/NixOS/patchelf/releases/download/0.18.0/patchelf-0.18.0-x86_64.tar.gz" \
             "$TOOLS/patchelf.tar.gz"
    tar xzf "$TOOLS/patchelf.tar.gz" -C "$TOOLS"
    # El tarball trae `bin/patchelf` y `share/`, no un binario suelto.
    export PATH="$TOOLS/bin:$PATH"
    command -v patchelf >/dev/null 2>&1 || {
        echo "error: no se pudo dejar patchelf en el PATH" >&2; exit 1; }
fi

if [ ! -f "$TOOLS/linuxdeployqt6.py" ]; then
    descargar "https://raw.githubusercontent.com/gavv/linuxdeployqt6.py/master/linuxdeployqt6.py" \
             "$TOOLS/linuxdeployqt6.py"
fi

# --- 2. Compilar con las opciones que exige un AppImage ---------------------
# El prefijo tiene que ser /usr porque es donde el runtime de AppImage espera
# encontrar la aplicacion, y el RPATH tiene que ser relativo: si la aplicacion
# guardase rutas absolutas, solo funcionaria en la maquina donde se compilo.
# `unrar` viene de bin/, que no se versiona. En una maquina limpia no esta, y
# sin el paquete sale incompleto sin que nada falle al compilar; el aviso de
# CMake se dispararia, pero es mejor descargarlo y seguir. En CI esto es lo unico
# que funciona: el runner nunca tiene bin/.
if [ ! -x "$RAIZ/bin/unrar" ]; then
    echo "==> descargando unrar (falta en bin/)"
    bash "$RAIZ/tools/fetch-rar.sh" "$RAIZ/bin"
fi

echo "==> compilando ($VERSION)"
rm -rf "$APPDIR"
cmake -S "$RAIZ" -B "$BUILD" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DQTRAR_FETCH_RAR=OFF \
    -DQTRAR_BUNDLE_UNRAR=ON \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON \
    -DCMAKE_INSTALL_RPATH='$ORIGIN/../lib' > /dev/null
cmake --build "$BUILD" --parallel > /dev/null

# --- 3. Montar el AppDir ----------------------------------------------------
echo "==> instalando en el AppDir"
mkdir -p "$APPDIR"
cmake --install "$BUILD" --prefix "$APPDIR/usr" > /dev/null

# Los menus de Dolphin y Nautilus apuntan a rutas del sistema
# (/usr/share/kio, ~/.local/share/nautilus/scripts) que dentro de un AppImage
# no existen: ahi se usan las acciones del .desktop, que si viajan dentro. Fuera
# de la imagen, que ademas no ocupa espacio de mas.
rm -rf "$APPDIR/usr/share/kio" "$APPDIR/usr/share/nautilus"

install -m 0755 "$RAIZ/packaging/appimage/AppRun" "$APPDIR/AppRun"
install -m 0644 "$APPDIR/usr/share/applications/qtrar.desktop" "$APPDIR/qtrar.desktop"
install -m 0644 "$APPDIR/usr/share/icons/hicolor/scalable/apps/qtrar.svg" "$APPDIR/qtrar.svg"

# appimagetool espera un icono raster en la raiz y con el nombre del Icon=.
# Los SVG tambien valen, pero casi ningun escritorio los aplica a la lista de
# aplicaciones, asi que se rasteriza si hay alguna herramienta a mano.
for conversor in rsvg-convert convert magick inkscape; do
    if command -v "$conversor" >/dev/null 2>&1; then
        case "$conversor" in
            rsvg-convert) "$conversor" -w 256 -h 256 -o "$APPDIR/qtrar.png" "$APPDIR/qtrar.svg" ;;
            convert|magick) "$conversor" -background none -resize 256x256 "$APPDIR/qtrar.svg" "$APPDIR/qtrar.png" ;;
            inkscape) "$conversor" -w 256 -h 256 --export-type=png --export-filename="$APPDIR/qtrar.png" "$APPDIR/qtrar.svg" ;;
        esac
        echo "==> icono rasterizado con $conversor"
        break
    fi
done
[ -f "$APPDIR/qtrar.png" ] || echo "aviso: sin icono PNG; se usa el SVG (se ve mejor con librsvg2-bin instalado)"

# Las licencias de Qt (LGPL-3.0) y de 7-Zip viajan dentro porque sus bibliotecas
# van dentro: LGPL obliga a acompañar el texto y a permitir sustituir las
# bibliotecas por otras de la misma licencia.
install -m 0644 /usr/share/common-licenses/LGPL-3 "$APPDIR/usr/share/doc/qtrar/LGPL-3.txt"

# El Qt que instalan las distribuciones no incluye `mkspecs/modules/*.pri`, que
# es donde los SDK oficiales de Qt declaran que modulos y que plugins trae cada
# libreria. El desplegador lee solo ahi, asi que sin estos ficheros no encuentra
# ni un modulo y el AppImage sale vacio (744 KB en vez de 20 MB). Se escriben a
# mano los que QtRAR usa de verdad; en CI, donde hay un SDK de Qt completo,
# install-qt-action pone los suyos y esto no hace falta.
modulos_qt() {
    local dir="$SDK/mkspecs/modules"
    mkdir -p "$dir"
    # "platforms" es el que elige donde se dibuja (xcb, wayland, offscreen); sin
    # el, un equipo sin Qt instalado no abre nada.
    cat > "$dir/qt_lib_gui.pri" <<-EOF
	QT.gui.name = QtGui
	QT.gui.module = Qt6Gui
	QT.gui.plugin_types = platforms imageformats iconengines platforminputcontexts platformthemes
	EOF
    cat > "$dir/qt_lib_widgets.pri" <<-EOF
	QT.widgets.name = QtWidgets
	QT.widgets.module = Qt6Widgets
	QT.widgets.plugin_types = styles
	EOF
    for modulo in core network dbus; do
        # El nombre del modulo tiene que ser tal cual aparece en el fichero de
        # la libreria (`libQt6Gui.so.6` -> `Qt6Gui`), no como lo llama Qt.
        case "$modulo" in
            core) nombre=Qt6Core ;;
            network) nombre=Qt6Network ;;
            dbus) nombre=Qt6DBus ;;
        esac
        cat > "$dir/qt_lib_$modulo.pri" <<-EOF
	QT.$modulo.name = $nombre
	QT.$modulo.module = $nombre
	EOF
    done
}

# --- 4. Desplegar Qt --------------------------------------------------------
# El Qt del sistema no sirve: un AppImage tiene que funcionar en un equipo que
# no tenga Qt instalado. El desplegador copia las bibliotecas de Qt, sus
# plugins (platformas, formatos de imagen) y las dependencias no-Qt (xcb, etc.),
# y reescribe el RPATH de todo lo copiado.
echo "==> desplegando Qt"
if [ -n "${QTRAR_QT_SDK:-}" ]; then
    SDK="$QTRAR_QT_SDK"
elif [ -n "${QT_ROOT_DIR:-}" ]; then
    SDK="$QT_ROOT_DIR"
else
    # Qt instalado con el sistema: se imita la disposicion de un SDK de Qt con
    # enlaces simbolicos, que es lo que espera el desplegador.
    SDK="$BUILD/qtsdk"
    mkdir -p "$SDK"
    enlazar() { mkdir -p "$(dirname "$SDK/$1")"; ln -sfn "$2" "$SDK/$1"; }
    # `ldconfig` no esta en todas partes (contenedores minimos) y con `set -e`
    # una orden que no existe tumba el script, asi que se comprueba antes.
    LIBQT=""
    if command -v ldconfig >/dev/null 2>&1; then
        LIBQT="$(ldconfig -p | awk '/libQt6Core\.so\.6/ {print $NF; exit}')"
        LIBQT="${LIBQT%/*}"
    fi
    if [ -z "$LIBQT" ] || [ ! -d "$LIBQT" ]; then
        LIBQT="$(find /usr/lib /usr/lib64 -name 'libQt6Core.so.6' -printf '%h\n' 2>/dev/null | head -1)"
    fi
    [ -d "$LIBQT" ] || { echo "error: no se encuentra libQt6Core; instala Qt 6" >&2; exit 1; }

    PLUGQT="$(find /usr/lib /usr/lib64 -type d -name plugins -path '*qt6*' 2>/dev/null | head -1)"
    [ -d "$PLUGQT" ] || { echo "error: no se encuentra el directorio de plugins de Qt 6" >&2; exit 1; }

    enlazar lib "$LIBQT"
    enlazar plugins "$PLUGQT"
    enlazar qml "$LIBQT/qt6/qml"
    enlazar translations /usr/share/qt6/translations
    enlazar include /usr/include/x86_64-linux-gnu/qt6
    # `mkspecs` se copia como enlaces en vez de enlazarse entero: hace falta
    # escribir `mkspecs/modules/*.pri` y el directorio del sistema es de root.
    mkdir -p "$SDK/mkspecs"
    for entrada in "$LIBQT/qt6/mkspecs"/*; do
        # `modules` se salta: se rellena abajo con nuestros .pri.
        [ "$(basename "$entrada")" = modules ] && continue
        ln -sfn "$entrada" "$SDK/mkspecs/$(basename "$entrada")"
    done
    modulos_qt
    echo "==> Qt del sistema usado como SDK: $SDK"
fi

# Ojo: las opciones -out-* se usan tal cual, sin colgarlas de -out-dir, asi que
# hay que darlas completas (relativas caerian en el directorio de trabajo).
python3 "$TOOLS/linuxdeployqt6.py" -force -qtdir "$SDK" \
    -out-dir "$APPDIR/usr" \
    -out-exe-dir "$APPDIR/usr/bin" -out-lib-dir "$APPDIR/usr/lib" \
    -out-plugins-dir "$APPDIR/usr/plugins" -out-data-dir "$APPDIR/usr/share" \
    -no-qml -no-translations \
    "$APPDIR/usr/bin/$APPNAME" > "$BUILD/linuxdeploy.log" 2>&1 || {
        echo "error: fallo el despliegue de Qt; log completo:" >&2
        tail -40 "$BUILD/linuxdeploy.log" >&2
        exit 1
    }
tail -3 "$BUILD/linuxdeploy.log"

# --- 5. Construir la imagen -------------------------------------------------
mkdir -p "$SALIDA"
echo "==> construyendo $(basename "$APPIMAGE")"
if command -v desktop-file-validate >/dev/null 2>&1; then
    desktop-file-validate "$APPDIR/qtrar.desktop" || true
fi
# appimagetool es un AppImage: en contenedores sin FUSE hay que extraerlo.
# Su salida se guarda para poder enseña si falla: sin esto, un fallo aqui sale
# como un "command not found" sin pistas de la causa real.
log_appimagetool="$BUILD/appimagetool.log"
if ! APPIMAGE_EXTRACT_AND_RUN=1 "$QTRAR_APPIMAGETOOL" --no-appstream "$APPDIR" "$APPIMAGE" \
        > "$log_appimagetool" 2>&1; then
    echo "error: appimagetool no pudo construir la imagen. Su salida:" >&2
    tail -30 "$log_appimagetool" >&2
    exit 1
fi
chmod +x "$APPIMAGE"

# --- 6. Comprobar que la imagen funciona ------------------------------------
# Sin display: es exactamente el caso del usuario que la ha descargado para
# probarla desde una sesion ssh, y comprueba que dentro no falte ningun plugin.
echo "==> comprobando"
verificacion="$(env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM \
    APPIMAGE_EXTRACT_AND_RUN=1 "$APPIMAGE" --version 2>&1)" || {
        echo "error: la imagen recien construida no arranca:" >&2
        echo "$verificacion" >&2
        exit 1
    }
echo "    $verificacion"

# Ni `rar` ni claves de licencia pueden viajar dentro. `unrar` si, por la
# excepcion 3.a de la EULA de RARLAB.
if [ -e "$APPDIR/usr/bin/rar" ] || [ -e "$APPDIR/usr/bin/rarreg.key" ]; then
    echo "error: el AppImage contiene el binario `rar`, que su licencia prohibe" >&2
    exit 1
fi
[ -x "$APPDIR/usr/bin/unrar" ] || { echo "error: falta unrar dentro del AppImage" >&2; exit 1; }

echo
echo "listo: $APPIMAGE ($(du -h "$APPIMAGE" | cut -f1))"
