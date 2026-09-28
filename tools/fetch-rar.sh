#!/usr/bin/env bash
#
# Descarga los binarios oficiales de RAR para Linux x64 de rarlab.com.
#
# AVISO DE LICENCIA (ver licenses/rar-license.txt, EULA rarlab §3.b/§3.c):
#   - El binario `rar` NO puede distribuirse dentro de otro paquete de software.
#     Por eso `bin/` esta en .gitignore y este script es una herramienta de
#     DESARROLLO, no parte de la release de QtRAR.
#   - El binario `unrar` si puede distribuirse por separado (excepcion explicita
#     de la misma clausula), por lo que es el unico que puede empaquetarse
#     (opcion CMake QTRAR_BUNDLE_UNRAR).
#
# Uso: tools/fetch-rar.sh [directorio-destino]
set -euo pipefail

VERSION="${QTRAR_RAR_VERSION:-723}"
BASE_URL="https://www.rarlab.com/rar"
TARBALL="rarlinux-x64-${VERSION}.tar.gz"
# SHA-256 verificado del tarball oficial. Actualizar al subir de version.
SHA256="${QTRAR_RAR_SHA256:-}"

DEST="${1:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)/bin}"

if [[ -z "$SHA256" ]]; then
    case "$VERSION" in
        723) SHA256="759b4b6aa0d9f77131882162951193f3a0e54bf60e1d8dc4255aa308accab588" ;;
        730b1) SHA256="" ;;  # beta: el hash cambia, recalcular manualmente
        *) echo "error: version $VERSION desconocida; define QTRAR_RAR_SHA256" >&2; exit 2 ;;
    esac
fi

if [[ -z "$SHA256" ]]; then
    echo "aviso: sin SHA-256 fijado para la version $VERSION; se omite la verificacion" >&2
fi

mkdir -p "$DEST"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "==> Descargando $TARBALL"
curl -fsSL --retry 3 --max-time 180 -o "$TMP/$TARBALL" "$BASE_URL/$TARBALL"

if [[ -n "$SHA256" ]]; then
    echo "==> Verificando SHA-256"
    echo "$SHA256  $TMP/$TARBALL" | sha256sum -c - >/dev/null
    echo "    ok"
fi

echo "==> Extrayendo a $DEST"
# Se extrae el contenido de rar/ sin alterar el binario ni sus metadatos.
tar xzf "$TMP/$TARBALL" -C "$TMP"

for f in rar unrar; do
    install -m 0755 "$TMP/rar/$f" "$DEST/$f"
done
install -m 0644 "$TMP/rar/license.txt" "$DEST/license.txt"
install -m 0644 "$TMP/rar/acknow.txt" "$DEST/acknow.txt"

# default.sfx lo usa la creacion de autoextraccion (Fase 5).
if [[ -f "$TMP/rar/default.sfx" ]]; then
    install -m 0644 "$TMP/rar/default.sfx" "$DEST/default.sfx"
fi

echo "==> Listo:"
for f in rar unrar; do
    printf '    %-8s %s\n' "$f" "$("$DEST/$f" 2>&1 | head -1)"
done
