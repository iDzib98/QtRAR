// Localiza los binarios externos que QtRAR necesita.
//
// Resolucion de `unrar` y `rar` (en ese orden, con "rar" primero porque es el
// que el script de desarrollo deja en bin/):
//   1. Variable de entorno QTRAR_UNRAR / QTRAR_RAR
//   2. Ruta `tools/rarPath` elegida por el usuario en QtRAR (solo para `rar`)
//   3. Directorio de la aplicacion y sus subdirectorios habituales
//   4. <proyecto>/bin (arranque desde el arbol de fuentes)
//   5. $PATH
//
// `7z` (o `7za`/`7zz`) se busca solo en el sistema: es la alternativa libre
// para crear ZIP, ya que el `rar` de la CLI de Linux no soporta ese formato.
#pragma once

#include "Types.h"

#include <QString>
#include <QStringList>

namespace qtrar {

struct ExternalTools {
    QString unrarPath;  ///< Vacio si no se encuentra.
    QString rarPath;    ///< Vacio si no se encuentra (licencia de pago).
    QString sevenZipPath;///< Vacio si no se encuentra.
    QString rarVersion;
    QString unrarVersion;

    bool canRead() const { return !unrarPath.isEmpty() || !sevenZipPath.isEmpty(); }
    bool canWrite() const { return !rarPath.isEmpty() || !sevenZipPath.isEmpty(); }
    bool canWriteRar() const { return !rarPath.isEmpty(); }
    bool canWriteZip() const { return !sevenZipPath.isEmpty(); }
};

class BinaryLocator
{
public:
    /// Busca las herramientas. `appDir` es el directorio del ejecutable;
    /// si es vacio se deduce de QCoreApplication::applicationDirPath().
    static ExternalTools locate(const QString &appDir = {});

    /// Devuelve la primera version de linea de un binario, p.ej. "7.23".
    /// Devuelve una cadena vacia si no se puede ejecutar.
    static QString versionOf(const QString &binary, const QString &versionSwitch = {});

    /// Directorios donde `rar` busca una clave de licencia, en el orden que
    /// documenta el manual de rarlab. Solo informativa: QtRAR nunca lee ni
    /// escribe la clave.
    static QStringList licenseKeySearchPaths();
};

} // namespace qtrar
