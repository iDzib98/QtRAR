# Fixtures

Salidas **reales** de los binarios oficiales, capturadas con
`tools/capture-fixtures.sh`. Se versionan para que `ctest` funcione en
cualquier máquina y sin tener RAR instalado.

Las que consume un parser están **sin cabeceras ni comentarios**: son la salida
byte a byte del comando. No se puede añadir un `#Comentario` porque `unrar` no
trata `#` como comentario y el parser leería una entrada fantasma llamada
`#Comentario`. La procedencia de cada una está aquí y en el script.

Regenerar (necesita los binarios de `tools/fetch-rar.sh`):

    tools/capture-fixtures.sh

| Fixture | Comando que la produce | Qué comprueba |
| --- | --- | --- |
| `rar5.lt.txt` | `rar a -ma5` + `unrar lt -cfg- -y -c- -p-` | RAR5 normal, nombres con espacios, acentos, `:` y comillas |
| `solid.lt.txt` | `rar a -ma5 -s` + `unrar lt` | Archivo sólido |
| `crypt.lt.txt` | `rar a -ma5 -psecreto123` + `unrar lt` | Datos cifrados, cabecera visible |
| `crypthdr.lt.txt` | `rar a -ma5 -hpsecreto123` + `unrar lt` | Cabecera y datos cifrados |
| `symlink.lt.txt` | `rar a -ma5 -ol` + `unrar lt` | Enlace simbólico |
| `rar5.docs.lb.txt` | `unrar lb -cfg- -y -c- -p- rar5.rar 'docs/*'` | Listado desnudo con máscara, que es lo que usa la navegación interna |
| `multivolume.lt.txt` | `rar a -v100k` + `unrar lt multi.part1.rar` | Listado de la primera parte de una serie |
| `multivolume_missing.txt` | la misma serie con la última parte borrada + `unrar t` | `Cannot find volume ...` |
| `test7z.7zsl.txt` | `7z l -slt -y` sobre un ZIP | Listado técnico de 7-Zip |
| `zip_no_rar.txt` | `unrar lt` sobre un ZIP | `unrar` sale con 0 pero avisa de que no es RAR |
| `missing_archive.txt` | `unrar lt` de un archivo inexistente | `Cannot open ...` con código 10 |
| `uso_comando.txt` | `unrar d` | Orden no soportada: imprime el uso y sale con 7 |
| `no_match.txt` | `unrar lb` con una máscara que no coincide | Archivo válido, salida vacía |
| `rar_add_progress.txt` | `rar a -ma5` | Líneas reales de progreso con porcentajes |

## Comprobado con RAR 7.23

- Las partes de una serie se llaman `multi.part1.rar`, `multi.part2.rar`…
  **sin cero a la izquierda**, y **no existe** un `multi.rar` al que abrir.
  El nombre antiguo (`multi.rar` + `multi.r01`) solo se acepta por compatibilidad.
- `unrar` expone `e l p t v x`. **No** admite `d` (borrar) ni renombrar.
- `unrar` **no** lee ZIP: sale con código 0 diciendo `is not RAR archive`.
- `rar` es evaluation y durante sus 40 días sí permite crear y modificar.
- `7z l -slt` **no** debe llevar `-bso0 -bse0`: p7zip los tolera pero el 7-Zip
  oficial suprime de verdad la salida estándar y el listado llega vacío.
