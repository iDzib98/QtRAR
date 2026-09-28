#!/usr/bin/env python3
"""Fuerza las traducciones de las etiquetas cortas de la barra y de los textos
de menu que dependen de ellas, en los dos .ts y sin mirar el valor anterior.

Se usa en vez de parchear a mano porque `lupdate` respeta lo que ya hay escrito:
si un .ts se deja sucio, un parche posterior que solo toque las entradas
"unfinished" no lo arregla, y los dos idiomas se acaban intercambiando.
"""
import html
import re
import sys
from xml.sax.saxutils import escape

# Todo lo que la app deberia decir en cada idioma, por texto de origen.
ES = {
    "Abrir": "Abrir", "Extraer en": "Extraer en", "Comprobar": "Comprobar",
    "Ver": "Ver", "Eliminar": "Eliminar", "Buscar": "Buscar", "Asistente": "Asistente",
    "Información": "Información", "Buscar virus": "Buscar virus", "Comentario": "Comentario",
    "Proteger": "Proteger", "Auto extraíble": "Auto extraíble", "Reparar": "Reparar",
    "Actualizar": "Actualizar", "Cifrar nombres": "Cifrar nombres",
    "Atrás": "Atrás", "Subir": "Subir",
    "&Abrir": "&Abrir", "&Añadir": "&Añadir", "Extraer en...": "Extraer en...",
    "&Comprobar": "&Comprobar", "&Ver": "&Ver", "&Eliminar...": "&Eliminar...",
    "&Buscar...": "&Buscar...", "&Asistente...": "&Asistente...",
    "&Información": "&Información", "Buscar &virus...": "Buscar &virus...",
    "&Comentario...": "&Comentario...", "&Proteger archivo...": "&Proteger archivo...",
    "&Reparar...": "&Reparar...", "Act&ualizar": "Act&ualizar",
    "Reparar archivo": "Reparar archivo",
    "Solo los archivos RAR se pueden reparar.": "Solo los archivos RAR se pueden reparar.",
    "Archivos RAR (*.rar);;Todos los archivos (*)": "Archivos RAR (*.rar);;Todos los archivos (*)",
    "Ruta actual": "Ruta actual",
    "Carpeta de destino (si no existe, se creará)": "Carpeta de destino (si no existe, se creará)",
    "Mostrar": "Mostrar", "Nueva carpeta": "Nueva carpeta",
    "Modo de actualización": "Modo de actualización",
    "Extraer y reemplazar": "Extraer y reemplazar",
    "Extraer y actualizar": "Extraer y actualizar", "Solo actualizar": "Solo actualizar",
    "Modo sobrescribir": "Modo sobrescribir",
    "Omitir los archivos existentes": "Omitir los archivos existentes",
    "Renombrar automáticamente": "Renombrar automáticamente", "Varios": "Varios",
    "Conservar archivos dañados": "Conservar archivos dañados",
    "Mostrar archivos en el explorador al terminar": "Mostrar archivos en el explorador al terminar",
    "Guardar opciones": "Guardar opciones", "General": "General",
    "Carpeta de extracción": "Carpeta de extracción",
    "Extraer en una subcarpeta del archivo": "Extraer en una subcarpeta del archivo",
    "Extraer directamente en la carpeta de destino": "Extraer directamente en la carpeta de destino",
    "Rutas de carpeta": "Rutas de carpeta", "Contraseña": "Contraseña",
    "Contraseña:": "Contraseña:", "Avanzado": "Avanzado",
    "Recordar estas opciones para futuras extracciones": "Recordar estas opciones para futuras extracciones",
    "La carpeta de destino y las opciones de extracción se pueden guardar desde la pestaña General.": "La carpeta de destino y las opciones de extracción se pueden guardar desde la pestaña General.",
    "Opciones": "Opciones", "Cancelar": "Cancelar",
    "Elige la carpeta de destino y ajusta las opciones de extracción en las pestañas General y Avanzado.": "Elige la carpeta de destino y ajusta las opciones de extracción en las pestañas General y Avanzado.",
    "Archivo: %1": "Archivo: %1", "Extraer en %1": "Extraer en %1",
    "Extraer en %1\\": "Extraer en %1\\", "Nombre de la carpeta:": "Nombre de la carpeta:",
    "No se pudo crear la carpeta en el destino seleccionado.": "No se pudo crear la carpeta en el destino seleccionado.",
    "Confirmar sobrescritura": "Confirmar sobrescritura", "Ayuda": "Ayuda",
    "\n... y %1 más": "\n... y %1 más",
    "Ya existen %1 archivos en el destino:\n\n%2\n\n¿Quieres sobrescribirlos?":
        "Ya existen %1 archivos en el destino:\n\n%2\n\n¿Quieres sobrescribirlos?",
    "No se pudo abrir %1 con la aplicación asociada.": "No se pudo abrir %1 con la aplicación asociada.",
    "&Ingresar licencia de RAR...": "&Ingresar licencia de RAR...",
    "Licencia de RAR": "Licencia de RAR",
    "Para registrar RAR, adquiere una licencia en RARLAB y coloca el archivo <code>rarreg.key</code> en una de las ubicaciones que busca RAR. Después, pulsa «Comprobar de nuevo». QtRAR no lee, copia ni escribe el contenido de la clave; es el propio RAR quien la valida.": "Para registrar RAR, adquiere una licencia en RARLAB y coloca el archivo <code>rarreg.key</code> en una de las ubicaciones que busca RAR. Después, pulsa «Comprobar de nuevo». QtRAR no lee, copia ni escribe el contenido de la clave; es el propio RAR quien la valida.",
    "Ubicaciones que comprueba RAR": "Ubicaciones que comprueba RAR",
    "Abrir carpeta": "Abrir carpeta", "RARLAB": "RARLAB",
    "Comprobar de nuevo": "Comprobar de nuevo", "Cerrar": "Cerrar",
    "<b>Modo de evaluación</b><br>RARLAB permite crear y modificar archivos durante los 40 días de prueba. Después se requiere una licencia de pago.": "<b>Modo de evaluación</b><br>RARLAB permite crear y modificar archivos durante los 40 días de prueba. Después se requiere una licencia de pago.",
    "<b>RAR parece estar registrado</b><br>El binario no muestra el aviso de evaluación.": "<b>RAR parece estar registrado</b><br>El binario no muestra el aviso de evaluación.",
    "<b>No se pudo comprobar la licencia</b><br>Instala el binario oficial <code>rar</code> para comprobar el estado.": "<b>No se pudo comprobar la licencia</b><br>Instala el binario oficial <code>rar</code> para comprobar el estado.",
    "Se encontró <code>rarreg.key</code> en:<br>%1<br>QtRAR solo comprueba que existe; RAR valida su contenido.": "Se encontró <code>rarreg.key</code> en:<br>%1<br>QtRAR solo comprueba que existe; RAR valida su contenido.",
    "No se encontró <code>rarreg.key</code> en las ubicaciones indicadas.": "No se encontró <code>rarreg.key</code> en las ubicaciones indicadas.",
    "Archivo(s) RAR o ZIP; acción contextual según las opciones.": "Archivo(s) RAR o ZIP; acción contextual según las opciones.",
    "Comprueba la integridad del archivo y termina.": "Comprueba la integridad del archivo y termina.",
    "Extrae el archivo junto a su ubicación y termina.": "Extrae el archivo junto a su ubicación y termina.",
    "Abre el diálogo para extraer el archivo.": "Abre el diálogo para extraer el archivo.",
    "Abre el diálogo para añadir los archivos seleccionados.": "Abre el diálogo para añadir los archivos seleccionados.",
    "qtrar: selecciona una sola acción de archivo.": "qtrar: selecciona una sola acción de archivo.",
    "qtrar: esta acción requiere exactamente un archivo.": "qtrar: esta acción requiere exactamente un archivo.",
    "qtrar: --add-to-archive requiere archivos seleccionados.": "qtrar: --add-to-archive requiere archivos seleccionados.",
    "Configurar binario &RAR...": "Configurar binario &RAR...",
    "RAR no disponible": "RAR no disponible",
    "No se pudo ejecutar el binario RAR seleccionado.": "No se pudo ejecutar el binario RAR seleccionado.",
    "Binario RAR configurado: %1": "Binario RAR configurado: %1",
    "Configurar el binario RAR": "Configurar el binario RAR",
    "Para crear y modificar archivos RAR, QtRAR necesita el ejecutable oficial <code>rar</code> de RARLAB. Descarga el paquete de Linux, extráelo y selecciona el ejecutable <code>rar</code> (no <code>unrar</code>). QtRAR guardará la ruta para las próximas ejecuciones; no descarga ni redistribuye ese binario.": "Para crear y modificar archivos RAR, QtRAR necesita el ejecutable oficial <code>rar</code> de RARLAB. Descarga el paquete de Linux, extráelo y selecciona el ejecutable <code>rar</code> (no <code>unrar</code>). QtRAR guardará la ruta para las próximas ejecuciones; no descarga ni redistribuye ese binario.",
    "Ruta al ejecutable rar": "Ruta al ejecutable rar", "Examinar…": "Examinar…",
    "Descargar desde RARLAB…": "Descargar desde RARLAB…", "Usar este binario": "Usar este binario",
    "Seleccionar el ejecutable rar": "Seleccionar el ejecutable rar",
    "Ejecutable RAR (rar);;Todos los archivos (*)": "Ejecutable RAR (rar);;Todos los archivos (*)",
    "Binario RAR no válido": "Binario RAR no válido",
    "Selecciona un archivo ejecutable llamado rar.": "Selecciona un archivo ejecutable llamado rar.",
    "No se reconoce el binario": "No se reconoce el binario",
    "El ejecutable seleccionado no parece ser RAR para Linux.": "El ejecutable seleccionado no parece ser RAR para Linux.",
}
EN = {
    "Abrir": "Open", "Extraer en": "Extract to", "Comprobar": "Test",
    "Ver": "View", "Eliminar": "Delete", "Buscar": "Find", "Asistente": "Wizard",
    "Información": "Information", "Buscar virus": "Scan for virus", "Comentario": "Comment",
    "Proteger": "Protect", "Auto extraíble": "SFX", "Reparar": "Repair",
    "Actualizar": "Refresh", "Cifrar names": "Encrypt names",
    "Cifrar nombres": "Encrypt names", "Atrás": "Back", "Subir": "Up",
    "&Abrir": "&Open", "&Añadir": "&Add", "Extraer en...": "Extract to...",
    "&Comprobar": "&Test", "&Ver": "&View", "&Eliminar...": "&Delete...",
    "&Buscar...": "F&ind...", "&Asistente...": "&Wizard...",
    "&Información": "&Info", "Buscar &virus...": "Scan for &virus...",
    "&Comentario...": "&Comment...", "&Proteger archivo...": "&Protect archive...",
    "&Reparar...": "&Repair...", "Act&ualizar": "Re&fresh",
    "Reparar archivo": "Repair archive",
    "Solo los archivos RAR se pueden reparar.": "Only RAR archives can be repaired.",
    "Archivos RAR (*.rar);;Todos los archivos (*)": "RAR archives (*.rar);;All files (*)",
    "Ruta actual": "Current path",
    "Carpeta de destino (si no existe, se creará)": "Destination folder (will be created if it does not exist)",
    "Mostrar": "Show", "Nueva carpeta": "New folder",
    "Modo de actualización": "Update mode",
    "Extraer y reemplazar": "Extract and replace",
    "Extraer y actualizar": "Extract and update", "Solo actualizar": "Update existing files only",
    "Modo sobrescribir": "Overwrite mode",
    "Omitir los archivos existentes": "Skip existing files",
    "Renombrar automáticamente": "Rename automatically", "Varios": "Miscellaneous",
    "Conservar archivos dañados": "Keep broken files",
    "Mostrar archivos en el explorador al terminar": "Show extracted files in file manager when done",
    "Guardar opciones": "Save options", "General": "General",
    "Carpeta de extracción": "Extraction folder",
    "Extraer en una subcarpeta del archivo": "Extract into a subfolder named after the archive",
    "Extraer directamente en la carpeta de destino": "Extract directly into the destination folder",
    "Rutas de carpeta": "Folder paths", "Contraseña": "Password",
    "Contraseña:": "Password:", "Avanzado": "Advanced",
    "Recordar estas opciones para futuras extracciones": "Remember these options for future extractions",
    "La carpeta de destino y las opciones de extracción se pueden guardar desde la pestaña General.": "The destination folder and extraction options can be saved from the General tab.",
    "Opciones": "Options", "Cancelar": "Cancel",
    "Elige la carpeta de destino y ajusta las opciones de extracción en las pestañas General y Avanzado.": "Choose a destination folder and adjust the extraction settings in the General and Advanced tabs.",
    "Archivo: %1": "Archive: %1", "Extraer en %1": "Extract to %1",
    "Extraer en %1\\": "Extract to %1\\", "Nombre de la carpeta:": "Folder name:",
    "No se pudo crear la carpeta en el destino seleccionado.": "Could not create the folder in the selected destination.",
    "Confirmar sobrescritura": "Confirm overwrite", "Ayuda": "Help",
    "\n... y %1 más": "\n... and %1 more",
    "Ya existen %1 archivos en el destino:\n\n%2\n\n¿Quieres sobrescribirlos?":
        "%1 files already exist in the destination:\n\n%2\n\nDo you want to overwrite them?",
    "No se pudo abrir %1 con la aplicación asociada.": "Could not open %1 with the associated application.",
    "&Ingresar licencia de RAR...": "&Enter RAR license...",
    "Licencia de RAR": "RAR license",
    "Para registrar RAR, adquiere una licencia en RARLAB y coloca el archivo <code>rarreg.key</code> en una de las ubicaciones que busca RAR. Después, pulsa «Comprobar de nuevo». QtRAR no lee, copia ni escribe el contenido de la clave; es el propio RAR quien la valida.": "To register RAR, obtain a license from RARLAB and place <code>rarreg.key</code> in one of the locations RAR searches. Then click “Check again”. QtRAR does not read, copy, or write the key; RAR itself validates it.",
    "Ubicaciones que comprueba RAR": "Locations checked by RAR",
    "Abrir carpeta": "Open folder", "RARLAB": "RARLAB",
    "Comprobar de nuevo": "Check again", "Cerrar": "Close",
    "<b>Modo de evaluación</b><br>RARLAB permite crear y modificar archivos durante los 40 días de prueba. Después se requiere una licencia de pago.": "<b>Evaluation mode</b><br>RARLAB allows creating and modifying archives during the 40-day trial. A paid license is required after that.",
    "<b>RAR parece estar registrado</b><br>El binario no muestra el aviso de evaluación.": "<b>RAR appears to be registered</b><br>The binary does not show the evaluation notice.",
    "<b>No se pudo comprobar la licencia</b><br>Instala el binario oficial <code>rar</code> para comprobar el estado.": "<b>Could not check the license</b><br>Install the official <code>rar</code> binary to check its status.",
    "Se encontró <code>rarreg.key</code> en:<br>%1<br>QtRAR solo comprueba que existe; RAR valida su contenido.": "A <code>rarreg.key</code> file was found at:<br>%1<br>QtRAR only checks that it exists; RAR validates its contents.",
    "No se encontró <code>rarreg.key</code> en las ubicaciones indicadas.": "No <code>rarreg.key</code> was found in the listed locations.",
    "Archivo(s) RAR o ZIP; acción contextual según las opciones.": "RAR or ZIP archive(s); contextual action depends on the options.",
    "Comprueba la integridad del archivo y termina.": "Test the archive's integrity and exit.",
    "Extrae el archivo junto a su ubicación y termina.": "Extract the archive alongside it and exit.",
    "Abre el diálogo para extraer el archivo.": "Open the extraction dialog for the archive.",
    "Abre el diálogo para añadir los archivos seleccionados.": "Open the dialog to add the selected files to an archive.",
    "qtrar: selecciona una sola acción de archivo.": "qtrar: choose only one archive action.",
    "qtrar: esta acción requiere exactamente un archivo.": "qtrar: this action requires exactly one file.",
    "qtrar: --add-to-archive requiere archivos seleccionados.": "qtrar: --add-to-archive requires selected files.",
    "Configurar binario &RAR...": "Configure RAR binary...",
    "RAR no disponible": "RAR unavailable",
    "No se pudo ejecutar el binario RAR seleccionado.": "The selected RAR binary could not be run.",
    "Binario RAR configurado: %1": "RAR binary configured: %1",
    "Configurar el binario RAR": "Configure the RAR binary",
    "Para crear y modificar archivos RAR, QtRAR necesita el ejecutable oficial <code>rar</code> de RARLAB. Descarga el paquete de Linux, extráelo y selecciona el ejecutable <code>rar</code> (no <code>unrar</code>). QtRAR guardará la ruta para las próximas ejecuciones; no descarga ni redistribuye ese binario.": "To create and modify RAR archives, QtRAR needs RARLAB's official <code>rar</code> executable. Download the Linux package, extract it, and select the <code>rar</code> executable (not <code>unrar</code>). QtRAR will save the path for future runs; it does not download or redistribute that binary.",
    "Ruta al ejecutable rar": "Path to the rar executable", "Examinar…": "Browse…",
    "Descargar desde RARLAB…": "Download from RARLAB…", "Usar este binario": "Use this binary",
    "Seleccionar el ejecutable rar": "Select the rar executable",
    "Ejecutable RAR (rar);;Todos los archivos (*)": "RAR executable (rar);;All files (*)",
    "Binario RAR no válido": "Invalid RAR binary",
    "Selecciona un archivo ejecutable llamado rar.": "Select an executable file named rar.",
    "No se reconoce el binario": "Unrecognized binary",
    "El ejecutable seleccionado no parece ser RAR para Linux.": "The selected executable does not appear to be RAR for Linux.",
    # El dialogo de la barra: la fuente es la de espanol, como en el resto.
    "Pequeño (16 px)": "Small (16 px)", "Mediano (24 px)": "Medium (24 px)",
    "Grande (32 px)": "Large (32 px)", "Tamaño del icono:": "Icon size:",
}

for path, table in (("res/i18n/qtrar_es.ts", ES), ("res/i18n/qtrar_en.ts", EN)):
    doc = open(path, encoding="utf-8").read()
    changed = 0

    def rewrite(match):
        global changed
        block = match.group(0)
        source = html.unescape(re.search(r"<source>(.*?)</source>", block, re.S).group(1))
        if source not in table:
            return block
        current = re.search(r"<translation[^>]*>(.*?)</translation>", block, re.S)
        current = html.unescape(current.group(1)) if current else None
        if current == table[source] and 'type="unfinished"' not in block:
            return block
        changed += 1
        return re.sub(r"<translation[^>]*>.*?</translation>",
                      "<translation>%s</translation>" % escape(table[source]),
                      block, flags=re.S)

    doc = re.sub(r"<message>.*?</message>", rewrite, doc, flags=re.S)
    open(path, "w", encoding="utf-8").write(doc)
    print(f"{path}: {changed} corregidas")

sys.exit(0)
