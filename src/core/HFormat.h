// Formateo de datos para las columnas y la barra de estado, con el formato que
// usa WinRAR por defecto.
#pragma once

#include <QDateTime>
#include <QString>

namespace qtrar {

class HFormat
{
public:
    /// "1.428" / "1,23 MB" / "3,41 GB". WinRAR usa separador de miles y
    /// 2 decimales a partir de KB.
    static QString fileSize(qint64 bytes, bool withSeparator = true);

    /// Igual que fileSize pero devuelve cadena vacia si no hay dato (-1).
    static QString fileSizeOrEmpty(qint64 bytes);

    /// "25.09.26 15:14" - formato corto de la columna "Modificado".
    static QString fileDateTime(const QDateTime &dt);

    /// "25/09/2026 15:14:30" - formato largo para la barra de estado.
    static QString fileDateTimeLong(const QDateTime &dt);

    /// "2%" o cadena vacia.
    static QString percent(int value);

    /// "1 archivo, 2 carpetas" / "1 archivo, 2 carpetas (3 bytes, 1 byte)"
    static QString fileCountText(int files, int folders, qint64 bytes, qint64 packed);

    /// Compacta un numero grande: "1,2 M" para la barra de espacio libre.
    static QString compactNumber(qint64 value);
};

} // namespace qtrar
