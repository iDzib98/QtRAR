// Parser de la salida de `unrar lt` y del listado tecnico de 7z.
//
// Formato real verificado con unrar 7.23 (ver tests/fixtures/):
//
//   Archive: /ruta/al.rar
//   Details: RAR 5, solid
//    Volume number: 1
//    Volume offset: 0
//  Compressed size: 25949
// Uncompressed size: 42561
//
//          Name: readme.txt          <- las claves se alinean a la derecha
//          Type: File                   en un campo de 12 caracteres
//          Size: 4280
//   Packed size: 113
//         Ratio: 2%
//         mtime: 2026-09-25 15:14:30,849861011
//    Attributes: -rw-rw-r--
//         CRC32: B9E87FF9
//       Host OS: Unix
//   Compression: RAR 5.0(v50) -m5 -md=128k
//         Flags: encrypted          <- con espacio final
//
// Reglas que el parser respeta:
//  - La clave se recorta por ambos lados (viene alineada a la derecha).
//  - `Name` inicia una entrada nueva; el valor se parte por el PRIMER ": "
//    para no romper nombres que contienen dos puntos.
//  - Las claves desconocidas se ignoran, para tolerar versiones nuevas.
//  - Los directorios no traen Size ni Packed size: ambos quedan en -1.
#pragma once

#include "Types.h"

#include <QString>
#include <QStringList>
#include <QVector>

namespace qtrar {

class ListParser
{
public:
    /// Analiza la salida tecnica de unrar y/o el listado de 7z.
    /// `archivePath` se usa solo para rellenar ArchiveInfo::path.
    static ArchiveListing parseUnrarTechnical(const QString &output,
                                              const QString &archivePath = {});

    /// Analiza `unrar lb` (listado desnudo: una ruta por linea).
    /// Se usa para listar el contenido de una subcarpeta sin releer toda la
    /// cabecera tecnica, que es lo que WinRAR hace al navegar.
    static QStringList parseBareList(const QString &output);

    /// Analiza el listado tecnico de 7z (`7z l -slt`), con lineas
    /// "Clave = Valor". Se usa para ZIP, que unrar no sabe leer.
    static ArchiveListing parseSevenZip(const QString &output,
                                        const QString &archivePath = {});

    // Expuestos para test unitario.
    static QDateTime parseUnrarTime(const QString &value);
    static EntryType parseType(const QString &value);

private:
    static bool splitKeyValue(const QString &line, QString *key, QString *value);
};

} // namespace qtrar
