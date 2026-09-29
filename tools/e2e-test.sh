#!/usr/bin/env bash
# Prueba de extremo a extremo de QtRAR contra los binarios oficiales de RAR.
#
# No necesita pantalla: usa las sondas sin interfaz de la propia aplicacion,
# que ejecutan exactamente el mismo codigo que la ventana principal.
set -u

RAIZ="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD:-$RAIZ/build}"
QT="$BUILD/QtRAR"
RAR="${QTRAR_RAR:-$RAIZ/bin/rar}"
UNRAR="${QTRAR_UNRAR:-$RAIZ/bin/unrar}"
TRAB=$(mktemp -d)
trap 'if [ "${QTRAR_KEEP_TMP:-0}" = 1 ]; then echo "fixtures: $TRAB"; else rm -rf "$TRAB"; fi' EXIT
export QTRAR_RAR="$RAR" QTRAR_UNRAR="$UNRAR" QT_QPA_PLATFORM=offscreen

[ -x "$QT" ] || { echo "FALTA: no existe $QT (cmake --build $BUILD)"; exit 1; }
[ -x "$RAR" ] || { echo "FALTA: no existe $RAR (tools/fetch-rar.sh)"; exit 1; }

ok=0; fallos=0
comprobar() {   # comprobar <descripcion> <condicion-exit-code-esperado> <real>
    if [ "$2" = "$3" ]; then
        printf '  ok    %s\n' "$1"; ok=$((ok + 1))
    else
        printf '  FALLA %s (esperado %s, real %s)\n' "$1" "$2" "$3"; fallos=$((fallos + 1))
    fi
}

# --- Fixtures --------------------------------------------------------------
mkdir -p "$TRAB/src/sub"
printf 'hola\n'   > "$TRAB/src/a.txt"
printf 'adios\n'  > "$TRAB/src/sub/b.txt"
head -c 300000 /dev/urandom > "$TRAB/src/rand.bin"

( cd "$TRAB" && "$RAR" a -r -y -m3 prueba.rar src          >/dev/null 2>&1 )
( cd "$TRAB" && "$RAR" a -r -y -psecreto cifrado.rar src    >/dev/null 2>&1 )
( cd "$TRAB" && "$RAR" a -v100k -y multi.rar src            >/dev/null 2>&1 )
if command -v 7z >/dev/null; then
    ( cd "$TRAB" && 7z a -bso0 -bse0 prueba.zip src          >/dev/null 2>&1 )
fi
printf 'no soy un archivo\n' > "$TRAB/texto.txt"

echo "== Listar =="
salida=$("$QT" --dump "$TRAB/prueba.rar" 2>/dev/null)
comprobar "el RAR se lista"          "1"  "$(echo "$salida" | sed -n 's/.*exito=\([01]\).*/\1/p')"
comprobar "el RAR tiene 3 ficheros"  "3"  "$(echo "$salida" | sed -n 's/.*ficheros=\([0-9]*\).*/\1/p')"
comprobar "el RAR tiene 2 carpetas"  "2"  "$(echo "$salida" | sed -n 's/.*carpetas=\([0-9]*\).*/\1/p')"
comprobar "el total son 300011 bytes" "300011" "$(echo "$salida" | sed -n 's/.*total=\([0-9]*\).*/\1/p')"
comprobar "un texto suelto no es RAR" "2" "$("$QT" --dump "$TRAB/texto.txt" 2>/dev/null | sed -n 's/diagnostico=\([0-9]*\).*/\1/p')"
if [ -f "$TRAB/prueba.zip" ]; then
    comprobar "el ZIP se lista igual" "3" "$("$QT" --dump "$TRAB/prueba.zip" 2>/dev/null | sed -n 's/.*ficheros=\([0-9]*\).*/\1/p')"
fi

echo "== Probar =="
QTRAR_TEST=1 "$QT" "$TRAB/prueba.rar"  >/dev/null 2>&1; comprobar "RAR integro"        0 $?
QTRAR_TEST=1 "$QT" "$TRAB/cifrado.rar" >/dev/null 2>&1; comprobar "cifrado sin clave"   1 $?
QTRAR_TEST=1 QTRAR_PASSWORD=secreto "$QT" "$TRAB/cifrado.rar" >/dev/null 2>&1
comprobar "cifrado con clave" 0 $?
if [ -f "$TRAB/prueba.zip" ]; then
    QTRAR_TEST=1 "$QT" "$TRAB/prueba.zip" >/dev/null 2>&1; comprobar "ZIP integro" 0 $?
fi

echo "== Extraer =="
extraer() { rm -rf "$2"; mkdir -p "$2"; QTRAR_EXTRACT_TO="$2" QTRAR_EXTRACT_WHAT="${3:-}" "$QT" "$1" >/dev/null 2>&1; }

extraer "$TRAB/prueba.rar" "$TRAB/out1"
if diff -r "$TRAB/src" "$TRAB/out1/src" >/dev/null 2>&1; then
    comprobar "el RAR se extrae identico" 0 0; else comprobar "el RAR se extrae identico" 0 1; fi

extraer "$TRAB/cifrado.rar" "$TRAB/out2" ""
comprobar "cifrado sin clave falla" 1 $?
extraer "$TRAB/cifrado.rar" "$TRAB/out3" ""
QTRAR_PASSWORD=secreto QTRAR_EXTRACT_TO="$TRAB/out3" "$QT" "$TRAB/cifrado.rar" >/dev/null 2>&1
if diff -r "$TRAB/src" "$TRAB/out3/src" >/dev/null 2>&1; then
    comprobar "cifrado con clave se extrae" 0 0; else comprobar "cifrado con clave se extrae" 0 1; fi

extraer "$TRAB/prueba.rar" "$TRAB/out4" "src/sub/b.txt"
comprobar "un solo miembro" "$( [ -f "$TRAB/out4/src/sub/b.txt" ] && echo 0 || echo 1 )" 0

if [ -f "$TRAB/prueba.zip" ]; then
    extraer "$TRAB/prueba.zip" "$TRAB/out5"
    if diff -r "$TRAB/src" "$TRAB/out5/src" >/dev/null 2>&1; then
        comprobar "el ZIP se extrae identico" 0 0; else comprobar "el ZIP se extrae identico" 0 1; fi
fi

echo "== Linea de ordenes =="
rm -rf "$TRAB/out6"; mkdir -p "$TRAB/out6"
"$QT" --extract-to "$TRAB/out6" "$TRAB/prueba.rar" >/dev/null 2>&1
comprobar "--extract-to extrae" 0 $?
if diff -r "$TRAB/src" "$TRAB/out6/src" >/dev/null 2>&1; then
    comprobar "--extract-to copia bien" 0 0; else comprobar "--extract-to copia bien" 0 1; fi
"$QT" --extract-to "$TRAB/out6" >/dev/null 2>&1
comprobar "--extract-to sin archivo avisa" 1 $?
"$QT" -l en --dump "$TRAB/prueba.rar" >/dev/null 2>&1
comprobar "-l en no se toma por archivo" 0 $?
"$QT" --test -- "$TRAB/prueba.rar" >/dev/null 2>&1
comprobar "--test es una acción contextual válida" 0 $?
mkdir -p "$TRAB/contexto"
cp "$TRAB/prueba.rar" "$TRAB/contexto/contexto.rar"
"$QT" --extract-here -- "$TRAB/contexto/contexto.rar" >/dev/null 2>&1
comprobar "--extract-here extrae junto al archivo" 0 $?
if diff -r "$TRAB/src" "$TRAB/contexto/src" >/dev/null 2>&1; then
    comprobar "--extract-here conserva la estructura" 0 0
else
    comprobar "--extract-here conserva la estructura" 0 1
fi
"$QT" --extract-to-dialog >/dev/null 2>&1
comprobar "--extract-to-dialog sin archivo avisa" 1 $?
"$QT" --add-to-archive >/dev/null 2>&1
comprobar "--add-to-archive sin selección avisa" 1 $?

echo "== Version y ayuda sin servidor gráfico =="
# El constructor de QApplication carga el plugin de plataforma, así que estas
# opciones tienen que resolverlo solas: si alguien las rompe, el proceso aborta
# con "could not connect to display" en vez de imprimir la version.
for opcion in --version --help; do
    env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM "$QT" "$opcion" >/dev/null 2>&1
    comprobar "$opcion sin display" 0 $?
done
env -u DISPLAY -u WAYLAND_DISPLAY -u QT_QPA_PLATFORM "$QT" --version 2>/dev/null \
    | grep -q 'QtRAR'
comprobar "--version imprime el nombre" 0 $?

echo "== Crear =="
rm -f "$TRAB/nuevo.rar"
QTRAR_CREATE="$TRAB/nuevo.rar" QTRAR_CREATE_FILES="$TRAB/src/a.txt" "$QT" >/dev/null 2>&1
comprobar "se crea un RAR"     0 $?
comprobar "el RAR nuevo existe" 0 "$( [ -s "$TRAB/nuevo.rar" ] && echo 0 || echo 1 )"
comprobar "el RAR nuevo lista" 1 "$("$QT" --dump "$TRAB/nuevo.rar" 2>/dev/null | sed -n 's/.*exito=\([01]\).*/\1/p')"

echo "== Modificar (solo el binario rar puede) =="
cp "$TRAB/prueba.rar" "$TRAB/moda.rar"
QTRAR_RENAME_FROM=src/a.txt QTRAR_RENAME_TO=src/otro.txt "$QT" "$TRAB/moda.rar" >/dev/null 2>&1
comprobar "renombrar" 0 $?
comprobar "el nombre nuevo existe" 1 "$("$QT" --dump "$TRAB/moda.rar" 2>/dev/null | grep -c '^F .* src/otro\.txt$')"
comprobar "el nombre viejo no" 0 "$("$QT" --dump "$TRAB/moda.rar" 2>/dev/null | grep -c '^F .* src/a\.txt$')"
QTRAR_REMOVE=src/otro.txt "$QT" "$TRAB/moda.rar" >/dev/null 2>&1
comprobar "borrar" 0 $?
comprobar "el elemento ya no esta" 0 "$("$QT" --dump "$TRAB/moda.rar" 2>/dev/null | grep -c 'src/otro\.txt')"
comprobar "el archivo sigue integro" 0 "$(QTRAR_TEST=1 "$QT" "$TRAB/moda.rar" >/dev/null 2>&1; echo $?)"

echo "== Comentario =="
cp "$TRAB/prueba.rar" "$TRAB/concomentario.rar"
printf 'Hola\nSegunda linea.\n' > "$TRAB/nota.txt"
QTRAR_COMMENT_FILE="$TRAB/nota.txt" "$QT" "$TRAB/concomentario.rar" >/dev/null 2>&1
comprobar "escribir comentario" 0 $?
# La salida va codificada en %XX para que el comentario, que tiene saltos de
# linea, quepa en una sola linea y sea facil de comparar.
leido=$(QTRAR_READ_COMMENT=1 "$QT" "$TRAB/concomentario.rar" 2>/dev/null | sed -n 's/^comentario=//p')
comprobar "leer comentario" "Hola%0ASegunda%20linea." "$leido"
comprobar "sin comentario, vacio" 1 "$(QTRAR_READ_COMMENT=1 "$QT" "$TRAB/prueba.rar" 2>/dev/null | grep -c '^comentario=$')"
QTRAR_SNAPSHOT="$TRAB/comentario.png" "$QT" "$TRAB/concomentario.rar" \
    >"$TRAB/comentario-ui.txt" 2>&1
comprobar "el panel de comentario aparece al abrir" 1 \
    "$(grep -F -c 'COMENTARIO_VISIBLE: 1' "$TRAB/comentario-ui.txt")"
comprobar "el panel muestra el comentario del RAR" 1 \
    "$(grep -F -c 'texto=Hola%0ASegunda%20linea.' "$TRAB/comentario-ui.txt")"

echo "== Volumenes =="
QTRAR_VOLUMES=1 "$QT" "$TRAB/multi.part1.rar" >/dev/null 2>&1
comprobar "serie completa" 0 $?
rm -f "$TRAB/multi.part3.rar"
QTRAR_VOLUMES=1 "$QT" "$TRAB/multi.part1.rar" >/dev/null 2>&1
comprobar "serie incompleta" 1 $?
falta=$(QTRAR_VOLUMES=1 "$QT" "$TRAB/multi.part1.rar" 2>/dev/null | sed -n 's/.*falta=\([^ ]*\).*/\1/p')
comprobar "dice que falta part3" 1 "$(echo "$falta" | grep -c 'part3\.rar')"

echo
echo "----------------------------------------"
printf 'ok: %d   fallos: %d\n' "$ok" "$fallos"
[ "$fallos" -eq 0 ] || exit 1
