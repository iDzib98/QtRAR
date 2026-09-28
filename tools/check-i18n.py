#!/usr/bin/env python3
"""Comprueba que los textos que QtRAR elige en tiempo de ejecucion esten
registrados para `lupdate`.

`CommandRegistry` guarda el texto de cada accion en una tabla y lo traduce con
`tr(def.textKey.c_str())`, y `shortLabel()` usa
`QCoreApplication::translate("qtrar::CommandRegistry", "...")`. Ninguna de las
dos formas es un `tr("literal")` que `lupdate` pueda seguir, asi que los textos
se registran a mano en `src/core/TranslationCatalog.cpp` con
QT_TRANSLATE_NOOP. Si esa lista se queda atras, la cadena sigue apareciendo en
ingles en los menus y no hay ningun aviso del compilador.

Este script compara las dos listas y dice que falta.
"""
import re
import sys

REG = "src/ui/CommandRegistry.cpp"
CAT = "src/core/TranslationCatalog.cpp"

registry = open(REG, encoding="utf-8").read()
catalog = open(CAT, encoding="utf-8").read()

table = registry[registry.index("m_definitions = {"):]
table = table[:table.index("\n    };")]
text_keys = re.findall(r'\{Cmd\w+,\s*"((?:[^"\\]|\\.)*)"', table)
short_labels = re.findall(
    r'translate\("qtrar::CommandRegistry",\s*"((?:[^"\\]|\\.)*)"\)', registry)

registered = set(
    re.findall(r'QT_TRANSLATE_NOOP\("qtrar::CommandRegistry",\s*"((?:[^"\\]|\\.)*)"\)',
               catalog))

wanted = list(dict.fromkeys(text_keys + short_labels))
missing = [t for t in wanted if t.replace('\\"', '"') not in registered]
stale = [t for t in sorted(registered) if t.replace('\\"', '"') not in wanted]

for t in missing:
    print(f"FALTA en TranslationCatalog.cpp: {t!r}")
for t in stale:
    print(f"Sobra en TranslationCatalog.cpp: {t!r}")

if missing:
    print(f"\n{len(missing)} sin registrar: se verian en ingles y sin traducir.")
    sys.exit(1)
if stale:
    print(f"\n{len(stale)} sobran: `lupdate` los marcara como obsoletos.")
    sys.exit(1)

print(f"OK: {len(wanted)} textos de CommandRegistry registrados y sin sobras.")
