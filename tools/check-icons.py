#!/usr/bin/env python3
"""Comprueba que todos los iconos que pide el codigo esten en el recurso.

`ThemeManager::icon()` carga de `:/icons/<nombre>.svg` y, si el recurso no lo
tiene, cae a `QIcon::fromTheme()`, que en una Mesa de iconos normal tampoco lo
tiene. El boton se queda en blanco sin decir nada, y el SVG aparece en
`res/icons/` dando la impresion de que si.

Este script compara tres cosas: los iconos que usa el codigo, los ficheros de
`res/icons/` y los que acaban en el recurso compilado (sea `qt_add_resources()`
en CMakeLists.txt o `res/qtrar.qrc`, que hoy estan los dos).

Uso:  python3 tools/check-icons.py [directorio-de-build]
"""
import os
import re
import sys

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BUILD = sys.argv[1] if len(sys.argv) > 1 else os.path.join(RAIZ, "build")

ICONOS_RSRC = "res/icons"


def nombres_en_codigo():
    """Nombres que se pasan a ThemeManager::icon() o se declaran en la tabla.

    Los iconos de los botones viven en la tabla de `CommandRegistry` como el
    tercer literal de cada fila (`{CmdAddToArchive, "&Anadir", "add", ...}`), y
    los de ventanas y navegacion en llamadas `ThemeManager::icon("...")`.
    """
    usados = set()
    for raiz, _, ficheros in os.walk(os.path.join(RAIZ, "src")):
        for f in ficheros:
            if not f.endswith((".cpp", ".h")):
                continue
            texto = open(os.path.join(raiz, f), encoding="utf-8").read()
            usados |= set(re.findall(
                r'ThemeManager::icon\(\s*(?:QStringLiteral\()?"([a-z0-9-]+)"', texto))
            usados |= set(re.findall(r'iconName\s*=\s*"([a-z0-9-]+)"', texto))

    registro = os.path.join(RAIZ, "src/ui/CommandRegistry.cpp")
    texto = open(registro, encoding="utf-8").read()
    tabla = texto[texto.index("m_definitions = {"):]
    tabla = tabla[:tabla.index("\n    };")]
    for fila in re.finditer(r'\{Cmd\w+,\s*"(?:[^"\\]|\\.)*",\s*"([a-z0-9-]+)"', tabla):
        usados.add(fila.group(1))
    return usados


def en_recurso_compilado():
    """Nombres presentes en el .qrc que CMake genera para el ejecutable."""
    encontrados = set()
    candidatas = []
    for raiz, _, ficheros in os.walk(BUILD):
        if os.path.basename(raiz) in (".rcc", "rcc"):
            candidatas += [os.path.join(raiz, f) for f in ficheros if f.endswith(".qrc")]
    for ruta in candidatas:
        texto = open(ruta, encoding="utf-8").read()
        encontrados |= set(re.findall(r'icons/([a-z0-9-]+)\.svg', texto))
    return encontrados, candidatas


def main():
    usados = nombres_en_codigo()
    en_disco = {f[:-4] for f in os.listdir(os.path.join(RAIZ, ICONOS_RSRC)) if f.endswith(".svg")}
    compilados, candidatas = en_recurso_compilado()

    if not candidatas:
        print("AVISO: no se encontro ningun .qrc generado; ejecuta cmake primero.")
        return 0

    # Iconos de los que se Sirve la propia aplicacion: no hacen falta en el
    # recurso, asi que se listan aparte para no dar un falso positivo.
    construccion = {"app", "qtrar"}

    faltan_disco = sorted(u for u in usados if u not in en_disco)
    faltan_rsrc = sorted(u for u in usados if u not in compilados and u not in construccion)

    for n in faltan_disco:
        print(f"FALTA el fichero: {ICONOS_RSRC}/{n}.svg")
    for n in faltan_rsrc:
        print(f"FALTA en el recurso: {n}.svg esta en {ICONOS_RSRC} pero no se compila")
        print(f"    anadelo a la lista FILES de qt_add_resources() en CMakeLists.txt")

    if faltan_disco or faltan_rsrc:
        print(f"\n{len(faltan_disco) + len(faltan_rsrc)} icono(s) que el codigo pide y no existen.")
        return 1

    print(f"OK: {len(usados)} iconos usados por el codigo, todos en disco y en el recurso.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
