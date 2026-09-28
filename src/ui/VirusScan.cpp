#include "ui/VirusScan.h"

#include "core/ArchiveService.h"
#include "core/Diagnostics.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryDir>

namespace qtrar {

QStringList VirusScan::candidateScanners()
{
    // clamscan/clamdscan (ClamAV) y scanson son los que hay en Linux. Se
    // comprueba tambien el nombre sin ruta, que es como los encuentra el PATH.
    return {QStringLiteral("clamdscan"), QStringLiteral("clamscan"), QStringLiteral("scanson")};
}

QString VirusScan::findScanner() const
{
    for (const QString &name : candidateScanners()) {
        const QString found = QStandardPaths::findExecutable(name);
        if (!found.isEmpty())
            return found;
    }
    return {};
}

ProcessOutcome VirusScan::scanArchive(const ArchiveService &service, const QString &archivePath,
                                      const QString &password, const QString &scanner) const
{
    if (scanner.isEmpty()) {
        ProcessOutcome fail;
        fail.success = false;
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("No hay ningun analizador de virus disponible");
        return fail;
    }

    // Se analiza lo extraido, no el archivo comprimido: los antivirus no saben
    // mirar dentro de un RAR, y el contenido es lo que hay que proteger.
    QTemporaryDir work;
    if (!work.isValid()) {
        ProcessOutcome fail;
        fail.success = false;
        fail.diagnostic = Diagnostic::OpenError;
        fail.message = QStringLiteral("No se pudo crear la carpeta temporal");
        return fail;
    }

    ExtractOptions ex;
    ex.destination = work.path();
    ex.hasPassword = !password.isEmpty();
    ex.password = password;
    const ProcessOutcome extracted = service.extract(archivePath, ex);
    if (!extracted.success)
        return extracted;

    // `-r` para bajar a las subcarpetas. Se capturan stdout y stderr juntos:
    // clamdscan escribe el resumen por stderr y es donde esta el veredicto.
    QProcess proc;
    proc.setProgram(scanner);
    proc.setArguments({QStringLiteral("--recursive"), work.path()});
    proc.start();
    if (!proc.waitForFinished(600000)) {
        proc.kill();
        ProcessOutcome fail;
        fail.success = false;
        fail.diagnostic = Diagnostic::TimedOut;
        fail.message = QStringLiteral("El analizador de virus no termino a tiempo");
        return fail;
    }

    const QString output = QString::fromLocal8Bit(proc.readAllStandardOutput())
                           + QString::fromLocal8Bit(proc.readAllStandardError());

    ProcessOutcome result;
    result.success = true;   // Que el analizador termine bien no significa que este limpio.
    result.diagnostic = Diagnostic::Success;

    // clamdscan: 0 limpio, 1 virus, 2 error. clamscan usa los mismos codigos.
    if (proc.exitCode() == 1) {
        result.success = false;
        result.diagnostic = Diagnostic::ChecksumError;
        result.message = output.trimmed();
    } else if (proc.exitCode() != 0) {
        result.success = false;
        result.diagnostic = Diagnostic::UnknownError;
        result.message = output.trimmed();
    } else if (!output.trimmed().isEmpty()) {
        // Salida 0 con texto: clamav escribe aqui el resumen. Se conserva para
        // poder enseñarlo, pero el archivo se considera limpio.
        result.message = output.trimmed();
    }
    return result;
}

} // namespace qtrar
