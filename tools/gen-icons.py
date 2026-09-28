#!/usr/bin/env python3
"""Genera res/icons/*.svg: el set de iconos recreados del tema "original".

Los iconos son originales, dibujados aqui desde cero con formas simples y una
paleta comun. No se copia ningun recurso de WinRAR (que es marca registrada):
el objetivo es que el conjunto se lea igual, no que sea identico pixel a pixel.

Uso: tools/gen-icons.py [directorio-destino]
"""
import os
import sys

# Paleta comun a todos los iconos (16x16, 1 unidad = 1 pixel).
SPINE_DARK = "#1F3864"
SPINE = "#2B579A"
SPINE_LIGHT = "#3A6BB5"
PAGE = "#F2E6C4"
GOLD = "#E8A33D"
GREEN = "#3C9A3C"
RED = "#C0392B"
GREY = "#8A8A8A"
DARK = "#333333"

HEAD = ('<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16" '
        'width="16" height="16">')
TAIL = "</svg>\n"

ICONS = {}


def add(name, body):
    ICONS[name] = HEAD + body + TAIL


def book_stack():
    """Pila de tres libros: el motivo que identifica al programa."""
    return (
        f'<rect x="1.5" y="1.5" width="13" height="4" rx="0.8" fill="{SPINE_LIGHT}" '
        f'stroke="{SPINE_DARK}" stroke-width="0.7"/>'
        f'<rect x="2.2" y="2.2" width="11.6" height="1.2" fill="{PAGE}" opacity="0.85"/>'
        f'<rect x="1.5" y="5.8" width="13" height="4" rx="0.8" fill="{SPINE}" '
        f'stroke="{SPINE_DARK}" stroke-width="0.7"/>'
        f'<rect x="2.2" y="6.5" width="11.6" height="1.2" fill="{PAGE}" opacity="0.85"/>'
        f'<rect x="1.5" y="10.1" width="13" height="4.3" rx="0.8" fill="{SPINE_DARK}" '
        f'stroke="#16294A" stroke-width="0.7"/>'
        f'<rect x="2.2" y="10.8" width="11.6" height="1.3" fill="{PAGE}" opacity="0.85"/>'
        f'<rect x="1.5" y="1.5" width="1.6" height="12.9" fill="{GOLD}" opacity="0.9"/>'
    )


add("qtrar", book_stack())
add("app", book_stack())

# Carpeta abierta / cerrada.
add("folder",
    f'<path d="M1 4.5A1 1 0 0 1 2 3.5h3.4l1.2 1.4H14a1 1 0 0 1 1 1v6.6a1 1 0 0 1-1 1H2a1 1 0 0 1-1-1z" '
    f'fill="{GOLD}" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M1 7h14l-1.4 6.5H2.3z" fill="#F5C46B" stroke="#A9761F" stroke-width="0.7"/>')

add("folder-locked",
    f'<path d="M1 4.5A1 1 0 0 1 2 3.5h3.4l1.2 1.4H11v1.6H4.6L2.9 12H2z" '
    f'fill="{GOLD}" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M5.6 9V7.6a2.4 2.4 0 0 1 4.8 0V9h.7a.6.6 0 0 1 .6.6v3.2a.6.6 0 0 1-.6.6H5.3'
    f'a.6.6 0 0 1-.6-.6V9.6a.6.6 0 0 1 .6-.6z" fill="#F2C14E" stroke="#8A5E12" stroke-width="0.7"/>'
    f'<path d="M6.6 9.2V7.6a1.4 1.4 0 0 1 2.8 0v1.6" fill="none" stroke="#8A5E12" '
    f'stroke-width="0.9"/>')


def page(inner, fill=PAGE, stroke="#7A6A45"):
    return (f'<path d="M3.5 1.5h6L12.5 4.5v10H3.5z" fill="{fill}" stroke="{stroke}" '
            f'stroke-width="0.8"/><path d="M9.5 1.5v3h3" fill="#DDD0A8" stroke="{stroke}" '
            f'stroke-width="0.7"/>{inner}')


def lines(n=3, y0=6.2, step=1.7, color="#8A7A50", w=6.4):
    out = ""
    for i in range(n):
        y = y0 + i * step
        out += f'<rect x="4.8" y="{y:.1f}" width="{w}" height="0.8" fill="{color}" opacity="0.75"/>'
    return out


add("file", page(lines()))
add("document", page(lines(5, 5.6, 1.5, "#5A6B8C", 6.2)))
add("image",
    page(f'<circle cx="6.1" cy="6.4" r="1" fill="{GOLD}"/>'
         f'<path d="M4.6 11.6l2.6-3 1.7 1.9 1.5-1.7 1.9 2.8z" fill="{GREEN}"/>', "#FFFFFF"))
add("archive", page(f'<rect x="4.6" y="6.2" width="7" height="4" rx="0.5" fill="{SPINE}"/>'
                    f'<rect x="7.2" y="7.4" width="1.6" height="1.6" fill="{PAGE}"/>', "#F7F3E6"))
add("code",
    page(f'<path d="M6.4 6.6L4.6 8.6l1.8 2M9.6 6.6l1.8 2-1.8 2" fill="none" '
         f'stroke="{SPINE}" stroke-width="1" stroke-linecap="round" '
         f'stroke-linejoin="round"/>', "#F2F2F2"))
add("media", page(f'<path d="M9.4 4.9v4.6a1.5 1.5 0 1 1-1-1.4V6.2l-2.6.6v3.4a1.5 1.5 0 1 1-1-1.4'
                  f'V5.8z" fill="{SPINE}"/>', "#F2F2F2"))

# El candado se superpone a la esquina inferior derecha de los tipos con clave.
def locked(inner):
    return (inner
            + f'<path d="M10.2 10.4h4.2a.5.5 0 0 1 .5.5v2.7a.5.5 0 0 1-.5.5h-4.2a.5.5 0 0 1-.5-.5'
              f'v-2.7a.5.5 0 0 1 .5-.5z" fill="{GOLD}" stroke="#8A5E12" stroke-width="0.6"/>'
              f'<path d="M11.1 10.4V9.1a1.2 1.2 0 0 1 2.4 0v1.3" fill="none" stroke="#8A5E12" '
              f'stroke-width="0.8"/>')


for _n in ("file", "document", "image", "archive", "code", "media"):
    add(_n + "-locked", locked(ICONS[_n].replace(HEAD, "").replace(TAIL, "")))

# Acciones de la barra de herramientas.
add("open",
    f'<path d="M1.5 4.2A.9.9 0 0 1 2.4 3.3h3.2l1.2 1.5h6.8a.9.9 0 0 1 .9.9v6.1a.9.9 0 0 1-.9.9H2.4'
    f'a.9.9 0 0 1-.9-.9z" fill="#F5C46B" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M7.6 6.2l4.2 3.4-4.2 3.4z" fill="{GREEN}" stroke="#256A25" stroke-width="0.6"/>')

add("extract",
    f'<path d="M1.5 4.2A.9.9 0 0 1 2.4 3.3h3.2l1.2 1.5h6.8a.9.9 0 0 1 .9.9v6.1a.9.9 0 0 1-.9.9H2.4'
    f'a.9.9 0 0 1-.9-.9z" fill="#F5C46B" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M8 6.1v4.2" stroke="{GREEN}" stroke-width="1.7" stroke-linecap="round"/>'
    f'<path d="M5.9 9.1L8 11.3l2.1-2.2" fill="none" stroke="{GREEN}" stroke-width="1.6" '
    f'stroke-linecap="round" stroke-linejoin="round"/>')

add("test",
    f'<circle cx="8" cy="8" r="6.2" fill="#EAF3E6" stroke="{GREEN}" stroke-width="1.1"/>'
    f'<path d="M5.1 8.2l2.1 2.2 3.9-4.4" fill="none" stroke="{GREEN}" stroke-width="1.7" '
    f'stroke-linecap="round" stroke-linejoin="round"/>')

add("view",
    f'<path d="M1 8s2.6-4.4 7-4.4S15 8 15 8s-2.6 4.4-7 4.4S1 8 1 8z" fill="#D6E9FA" '
    f'stroke="{SPINE}" stroke-width="1"/>'
    f'<circle cx="8" cy="8" r="2.2" fill="{SPINE}"/>')

add("delete",
    f'<rect x="5.6" y="2.2" width="4.8" height="1.5" rx="0.5" fill="{GREY}"/>'
    f'<path d="M3.4 4.4h9.2l-.8 9a1 1 0 0 1-1 .9H5.2a1 1 0 0 1-1-.9z" fill="#D8D8D8" '
    f'stroke="#5A5A5A" stroke-width="0.9"/>'
    f'<path d="M6.5 6.6v5.4M8 6.6v5.4M9.5 6.6v5.4" stroke="#5A5A5A" stroke-width="0.9" '
    f'stroke-linecap="round"/>')

add("find",
    f'<circle cx="6.9" cy="6.9" r="4.3" fill="#D6E9FA" stroke="{SPINE}" stroke-width="1.2"/>'
    f'<path d="M10 10l3.9 3.9" stroke="{SPINE_DARK}" stroke-width="2" stroke-linecap="round"/>')

add("wizard",
    f'<path d="M8 1.4l1.5 4.1 4.1 1.5-4.1 1.5L8 12.6 6.5 8.5 2.4 7l4.1-1.5z" '
    f'fill="{GOLD}" stroke="#A9761F" stroke-width="0.8"/>'
    f'<rect x="3.4" y="13" width="9.2" height="1.8" rx="0.6" fill="{SPINE}"/>')

add("info",
    f'<circle cx="8" cy="8" r="6.2" fill="#D6E9FA" stroke="{SPINE}" stroke-width="1.1"/>'
    f'<circle cx="8" cy="4.9" r="1" fill="{SPINE}"/>'
    f'<rect x="7.3" y="6.6" width="1.4" height="4.8" rx="0.5" fill="{SPINE}"/>')

add("shield",
    f'<path d="M8 1.4l5.2 1.8v4.3c0 3.2-2.1 6-5.2 7.1-3.1-1.1-5.2-3.9-5.2-7.1V3.2z" '
    f'fill="#DCEEDC" stroke="{GREEN}" stroke-width="1.1"/>'
    f'<path d="M5.6 7.9l1.8 1.8 3.1-3.5" fill="none" stroke="{GREEN}" stroke-width="1.5" '
    f'stroke-linecap="round" stroke-linejoin="round"/>')

add("protect",
    f'<path d="M8 1.4l5.2 1.8v4.3c0 3.2-2.1 6-5.2 7.1-3.1-1.1-5.2-3.9-5.2-7.1V3.2z" '
    f'fill="none" stroke="{SPINE}" stroke-width="1.2"/>'
    f'<circle cx="8" cy="7.4" r="1.7" fill="{GOLD}" stroke="#8A5E12" stroke-width="0.8"/>'
    f'<rect x="7.4" y="8.5" width="1.2" height="2.6" fill="{GOLD}"/>')

add("comment",
    f'<path d="M2 3.4h12a.8.8 0 0 1 .8.8v6.4a.8.8 0 0 1-.8.8H7.2l-3.1 2.6v-2.6H2a.8.8 0 0 1-.8-.8'
    f'V4.2a.8.8 0 0 1 .8-.8z" fill="{PAGE}" stroke="#8A7A50" stroke-width="0.9"/>'
    f'<path d="M3.6 6h8.8M3.6 8.3h6" stroke="#8A7A50" stroke-width="0.8"/>')

add("encrypt",
    f'<rect x="3.4" y="7" width="7.2" height="6" rx="0.8" fill="{GOLD}" stroke="#8A5E12" '
    f'stroke-width="0.9"/>'
    f'<path d="M5.1 7V5.2a1.9 1.9 0 0 1 3.8 0V7" fill="none" stroke="#8A5E12" stroke-width="1.1"/>'
    f'<circle cx="7" cy="9.8" r="1" fill="#8A5E12"/>')

add("sfx",
    f'<rect x="1.5" y="3" width="13" height="10" rx="1" fill="#4A4A4A"/>'
    f'<rect x="3" y="4.4" width="10" height="4.2" fill="#1E1E1E"/>'
    f'<path d="M5.4 7.4h1.1l.8-1.4.9 2.6.7-1.2h1.7" fill="none" stroke="{GREEN}" '
    f'stroke-width="0.9" stroke-linecap="round"/>'
    f'<rect x="5.4" y="10" width="5.2" height="1.6" rx="0.4" fill="{GREY}"/>')

add("add",
    f'<path d="M1.5 4.2A.9.9 0 0 1 2.4 3.3h3.2l1.2 1.5h6.8a.9.9 0 0 1 .9.9v6.1a.9.9 0 0 1-.9.9H2.4'
    f'a.9.9 0 0 1-.9-.9z" fill="#F5C46B" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M8 6.2v4.4M5.8 8.4h4.4" stroke="{GREEN}" stroke-width="1.7" stroke-linecap="round"/>')

add("newarchive",
    f'<path d="M2 3.2h8.4L13 5.8V12a.6.6 0 0 1-.6.6H2a.6.6 0 0 1-.6-.6V3.8A.6.6 0 0 1 2 3.2z" '
    f'fill="#F7F3E6" stroke="#7A6A45" stroke-width="0.8"/>'
    f'<rect x="3.4" y="7" width="5.4" height="3.2" rx="0.4" fill="{SPINE}"/>'
    f'<path d="M11.6 9.4v4.2M9.5 11.5h4.2" stroke="{GREEN}" stroke-width="1.5" '
    f'stroke-linecap="round"/>')

add("rename",
    f'<path d="M2 12.6l.7-2.6 7.1-7.1 1.9 1.9-7.1 7.1z" fill="{PAGE}" stroke="#7A6A45" '
    f'stroke-width="0.8"/>'
    f'<path d="M10.4 2.2l1.9 1.9 1-.9-1.9-1.9z" fill="{GOLD}" stroke="#8A5E12" '
    f'stroke-width="0.7"/>'
    f'<path d="M2 12.6h4" stroke="{SPINE}" stroke-width="0.9"/>')

add("goto",
    f'<path d="M2 8h8" stroke="{SPINE}" stroke-width="1.5" stroke-linecap="round"/>'
    f'<path d="M7.4 5.2L11 8l-3.6 2.8z" fill="{SPINE}"/>'
    f'<path d="M12.4 3.4v9.2" stroke="{GOLD}" stroke-width="1.6" stroke-linecap="round"/>')

add("selectall",
    f'<path d="M2 3.2h8.4L13 5.8V12a.6.6 0 0 1-.6.6H2a.6.6 0 0 1-.6-.6V3.8A.6.6 0 0 1 2 3.2z" '
    f'fill="#F7F3E6" stroke="#7A6A45" stroke-width="0.8"/>'
    f'<path d="M3.4 8.8l1.8 1.9 3.4-3.8" fill="none" stroke="{GREEN}" stroke-width="1.5" '
    f'stroke-linecap="round" stroke-linejoin="round"/>')

add("deselectall",
    f'<path d="M2 3.2h8.4L13 5.8V12a.6.6 0 0 1-.6.6H2a.6.6 0 0 1-.6-.6V3.8A.6.6 0 0 1 2 3.2z" '
    f'fill="#F7F3E6" stroke="#7A6A45" stroke-width="0.8"/>'
    f'<path d="M3.6 7.4l4.4 4.4M8 7.4L3.6 11.8" stroke="{RED}" stroke-width="1.5" '
    f'stroke-linecap="round"/>')

add("invert",
    f'<rect x="2.2" y="3.4" width="8.4" height="9" rx="0.6" fill="#F7F3E6" stroke="#7A6A45" '
    f'stroke-width="0.8"/>'
    f'<path d="M2.2 3.4h4.2v9H2.2z" fill="{SPINE}" opacity="0.85"/>')

add("options",
    f'<path d="M8 5.4a2.6 2.6 0 1 0 0 5.2 2.6 2.6 0 0 0 0-5.2zm0 1.6a1 1 0 1 1 0 2 1 1 0 0 1 0-2z" '
    f'fill="{SPINE}"/>'
    f'<path d="M6.9 1.6h2.2l.3 1.7 1.4.8 1.6-.7 1.1 1.9-1.3 1.2v1.6l1.3 1.2-1.1 1.9-1.6-.7'
    f'-1.4.8-.3 1.7H6.9l-.3-1.7-1.4-.8-1.6.7-1.1-1.9 1.3-1.2V6.5L2.5 5.3l1.1-1.9 1.6.7'
    f'1.4-.8z" fill="none" stroke="{SPINE}" stroke-width="0.9" stroke-linejoin="round"/>')

add("about",
    f'<circle cx="8" cy="8" r="6.2" fill="#D6E9FA" stroke="{SPINE}" stroke-width="1.1"/>'
    f'<path d="M6.6 7.2h1.5v4H6.6z" fill="{SPINE}"/>'
    f'<circle cx="7.35" cy="5.4" r="0.95" fill="{SPINE}"/>')

add("help",
    f'<path d="M2.4 3.2h11.2v9.6H2.4z" fill="#F7F3E6" stroke="#7A6A45" stroke-width="0.8"/>'
    f'<path d="M5.2 5.9a2.8 2.8 0 0 1 5.4.9c0 1.6-2.6 1.9-2.6 3.4" fill="none" '
    f'stroke="{SPINE}" stroke-width="1.4" stroke-linecap="round"/>'
    f'<circle cx="8" cy="11.4" r="0.95" fill="{SPINE}"/>')

# Navegacion.
add("up",
    f'<path d="M1.5 4.2A.9.9 0 0 1 2.4 3.3h3.2l1.2 1.5h6.8a.9.9 0 0 1 .9.9v6.1a.9.9 0 0 1-.9.9H2.4'
    f'a.9.9 0 0 1-.9-.9z" fill="#F5C46B" stroke="#A9761F" stroke-width="0.8"/>'
    f'<path d="M8 11.6V6.9M5.9 8.9L8 6.7l2.1 2.2" fill="none" stroke="{SPINE}" '
    f'stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round"/>')

add("back",
    f'<path d="M9.4 4.2L5.2 8.2l4.2 4" fill="none" stroke="{SPINE}" stroke-width="1.6" '
    f'stroke-linecap="round" stroke-linejoin="round"/>'
    f'<path d="M5.4 8.2h6" stroke="{SPINE}" stroke-width="1.3" stroke-linecap="round"/>')

add("refresh",
    f'<path d="M12.6 8a4.6 4.6 0 1 1-1.5-3.4" fill="none" stroke="{SPINE}" stroke-width="1.5" '
    f'stroke-linecap="round"/>'
    f'<path d="M12.9 1.9v3.4H9.5" fill="none" stroke="{SPINE}" stroke-width="1.5" '
    f'stroke-linecap="round" stroke-linejoin="round"/>')

add("close",
    f'<path d="M3.6 3.6l8.8 8.8M12.4 3.6l-8.8 8.8" stroke="{DARK}" stroke-width="1.8" '
    f'stroke-linecap="round"/>')

add("password",
    f'<path d="M3.4 7h7.2a.6.6 0 0 1 .6.6v4.6a.6.6 0 0 1-.6.6H3.4a.6.6 0 0 1-.6-.6V7.6'
    f'a.6.6 0 0 1 .6-.6z" fill="{GOLD}" stroke="#8A5E12" stroke-width="0.8"/>'
    f'<path d="M5.2 7V5.1a2.8 2.8 0 0 1 5.6 0V7" fill="none" stroke="#8A5E12" '
    f'stroke-width="1.1"/>'
    f'<circle cx="7" cy="9.6" r="1.1" fill="#8A5E12"/>')


def main():
    dest = sys.argv[1] if len(sys.argv) > 1 else \
        os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                     "res", "icons")
    os.makedirs(dest, exist_ok=True)
    for name, svg in sorted(ICONS.items()):
        with open(os.path.join(dest, name + ".svg"), "w", encoding="utf-8") as fh:
            fh.write(svg)
    print(f"{len(ICONS)} iconos escritos en {dest}")


if __name__ == "__main__":
    main()
