#!/usr/bin/env bash
#
# Genera las fixtures de test a partir de los binarios oficiales de RAR.
#
# Las fixtures son texto plano con salidas REALES de `unrar lt` de unrar 7.23.
# Se regeneran con tools/capture-fixtures.sh y se versionan, de forma que
# `ctest` funciona en cualquier maquina y sin tener RAR instalado.
#
# Uso: tools/capture-fixtures.sh [directorio-destino]
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${1:-$HERE/tests/fixtures}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

RAR="${QTRAR_RAR:-$HERE/bin/rar}"
UNRAR="${QTRAR_UNRAR:-$HERE/bin/unrar}"

if [[ ! -x "$RAR" || ! -x "$UNRAR" ]]; then
    echo "error: faltan los binarios. Ejecuta tools/fetch-rar.sh antes." >&2
    exit 1
fi

mkdir -p "$OUT"
SRC="$WORK/src"
mkdir -p "$SRC/docs" "$SRC/img/nested" "$SRC/empty"
cd "$SRC"

# Contenido sintetico: da nombres no ASCII, espacios, parentesis y dos puntos,
# que son los casos que rompen los parsers ingenuos.
printf 'contenido de prueba\n%.0s' {1..40} > readme.txt
printf 'manual\n' > docs/manual.txt
printf 'notas\n' > docs/notas.txt
printf 'acentos y espacios\n' > "img/año_español (1).txt"
head -c 20000 /dev/urandom > img/foto.png
head -c 5000 /dev/urandom > img/nested/deep.bin
printf 'x' > "docs/weird:name:colon.txt"
printf 'y' > "docs/weird'quote\"dquote.txt"
ln -s readme.txt docs/link.txt

capture() {
    local archive="$1" name="$2" extra_args="${3:-}"
    local a="$WORK/$name.rar"
    # shellcheck disable=SC2086
    "$RAR" a -ma5 $extra_args "$a" readme.txt docs img empty >/dev/null 2>&1 || true
    # Salida limpia, sin cabeceras: es lo que leeu el parser.
    "$UNRAR" lt -cfg- -y -c- -p- "$a" > "$OUT/$name.lt.txt" 2>&1 || true
}

capture x rar5
capture x solid "-s"
capture x crypt "-psecreto123"
capture x crypthdr "-hpsecreto123"
capture x symlink "-ol"

# Serie de volumenes. OJO, comprobado con rar 7.23: las partes se llaman
# `multi.part1.rar`, `multi.part2.rar`... SIN cero a la izquierda, y NO existe
# un `multi.rar` al que abrir. Por eso el listado va contra `part1`.
{
    # Ver tests/fixtures/README.md: la salida es limpia, sin comentarios.
    # Se necesitan varios volumenes: si el contenido cabe en uno, rar escribe
    # un unico `multi.rar` y no hay serie que inspeccionar.
    head -c 400000 /dev/urandom > "$WORK/volumen.bin"
    "$RAR" a -ma5 -v100k "$WORK/multi.rar" "$WORK/volumen.bin" >/dev/null 2>&1 || true
    "$UNRAR" lt -cfg- -y -c- -p- "$WORK/multi.part1.rar" 2>&1 || true
} > "$OUT/multivolume.lt.txt"

# Serie a la que le falta la ultima parte: es lo que unrar t dice entonces.
{
    rm -f "$WORK"/multi.rar "$WORK"/multi.part*.rar
    "$RAR" a -ma5 -v100k "$WORK/multi.rar" "$WORK/volumen.bin" >/dev/null 2>&1 || true
    LAST=$(find "$WORK" -maxdepth 1 -name 'multi.part*.rar' | sort -V | tail -1)
    if [ -n "$LAST" ] && [ "$(basename "$LAST")" != "multi.part1.rar" ]; then
        rm -f "$LAST"
    fi
    "$UNRAR" t -cfg- -y -c- -p- "$WORK/multi.part1.rar" 2>&1 || true
} > "$OUT/multivolume_missing.txt"

# Listado desnudo con mascara de subcarpeta (lo que usa la navegacion interna).
"$UNRAR" lb -cfg- -y -c- -p- "$WORK/rar5.rar" 'docs/*' > "$OUT/rar5.docs.lb.txt" 2>&1 || true

# Salida de un ZIP: unrar NO lo lee, 7z si. Se guardan ambas para documentar
# por que existe el motor ZIP.
if command -v 7z >/dev/null 2>&1; then
    7z a -tzip -bso0 -bsp0 "$WORK/test7z.zip" readme.txt docs >/dev/null 2>&1 || true
    # unrar sobre un ZIP: sale con 0 pero avisa de que no es RAR.
    "$UNRAR" lt -cfg- -y -c- -p- "$WORK/test7z.zip" > "$OUT/zip_no_rar.txt" 2>&1 || true
    # Sin -bso0/-bse0: p7zip los tolera, pero el 7-Zip oficial corta de
    # verdad la salida estandar y el listado -slt llegaria vacio.
    7z l -slt -y "$WORK/test7z.zip" > "$OUT/test7z.7zsl.txt" 2>&1 || true
fi

# Diagnosticos: casos trampa donde el codigo de salida miente. Cada uno en su
# propia fixture, porque `analyze()` decide por el primer indicio que encuentra.
# Archivo inexistente: exit 10 + "Cannot open ...".
"$UNRAR" lt -cfg- -y -c- -p- "$WORK/no_existe.rar" > "$OUT/missing_archive.txt" 2>&1 || true

# `unrar d`: orden que unrar 7.23 no soporta; imprime el uso y sale con 7.
"$UNRAR" d -cfg- -y -c- -p- "$WORK/rar5.rar" readme.txt > "$OUT/uso_comando.txt" 2>&1 || true

# Mascara que no coincide con nada: exit 0 y salida completamente vacia.
"$UNRAR" lb -cfg- -y -c- -p- "$WORK/rar5.rar" 'nada_de_esto*' > "$OUT/no_match.txt" 2>&1 || true

# Progreso: lineas reales de rar con porcentajes.
{
    "$RAR" a -ma5 "$WORK/progreso.rar" readme.txt img 2>&1 || true
} > "$OUT/rar_add_progress.txt"

echo "Fixtures escritas en $OUT:"
ls -1 "$OUT"
