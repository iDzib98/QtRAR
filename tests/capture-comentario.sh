#!/usr/bin/env bash
# Captura la salida de `unrar lt` de un archivo CON comentario, para probar el
# parser. Se ejecuta a mano; la fixture se versiona.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RAR="${QTRAR_RAR:-$HERE/bin/rar}"
UNRAR="${QTRAR_UNRAR:-$HERE/bin/unrar}"
WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT

cd "$WORK"
printf 'Hola\nEsta es la segunda linea.\n' > nota.txt
printf 'contenido\n' > fichero.txt
"$RAR" a -y simple.rar fichero.txt >/dev/null 2>&1
# Comprobado con RAR 7.23: `c` es "poner comentario" y `-z` pega el nombre
# del fichero de texto. Con un espacio, rar toma el nombre como el del archivo.
"$RAR" c -znota.txt simple.rar >/dev/null 2>&1

"$UNRAR" lt -cfg- -y simple.rar > "$HERE/tests/fixtures/comentario.lt.txt" 2>&1 || true
echo "Fixture escrita: tests/fixtures/comentario.lt.txt"
