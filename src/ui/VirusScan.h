#pragma once

#include "core/Diagnostics.h"

#include <QString>
#include <QStringList>

namespace qtrar {

class ArchiveService;

/// Analisis antivirus de un archivo, al estilo del "Analisis antivirus" de
/// WinRAR.
///
/// WinRAR no lleva antivirus dentro: llama al que tenga instalado el sistema.
/// Aqui se hace lo mismo, con la limitacion de que solo valen los que se
/// pueden manejar desde la linea de comandos. Si no hay ninguno, se dice
/// claramente en vez de dar un "todo correcto" que no se ha comprobado nada.
class VirusScan
{
public:
    /// Localiza un analizador utilizable en el PATH.
    /// Devuelve la ruta completa, o cadena vacia si no hay ninguno.
    QString findScanner() const;

    /// Extrae el archivo a un temporal y lo pasa por el analizador.
    /// `scanner` debe venir de `findScanner()`.
    ProcessOutcome scanArchive(const ArchiveService &service, const QString &archivePath,
                               const QString &password, const QString &scanner) const;

    /// Analizadores que se buscan, en este orden. El primero que exista gana.
    static QStringList candidateScanners();
};

} // namespace qtrar
