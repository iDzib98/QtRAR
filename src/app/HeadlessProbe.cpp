#include "HeadlessProbe.h"

#include "core/ArchiveService.h"
#include "core/BinaryLocator.h"
#include "core/Types.h"

#include <QDir>
#include <QTextStream>
#include <memory>

namespace qtrar {

namespace {

/// Un servicio con las herramientas ya localizadas. Casi todas las sondas lo
/// necesitan, y repetirlo en cada bloque era ruido. Se devuelve por puntero
/// porque `ArchiveService` es un QObject y no se puede mover ni copiar.
std::unique_ptr<ArchiveService> makeService()
{
    auto service = std::make_unique<ArchiveService>();
    service->setTools(BinaryLocator::locate());
    return service;
}

} // namespace

bool runHeadlessProbe(const ProbeRequest &request, int &exitCode)
{
    QTextStream out(stdout);
    const QString archive = QDir(request.archivePath).absolutePath();

    // --- Volcado del listado -------------------------------------------------
    if (request.dump) {
        const auto s = makeService();
        ProcessOutcome outcome;
        const ArchiveListing listing = s->list(archive, {}, false, &outcome);
        out << "diagnostico=" << int(outcome.diagnostic) << " exito=" << outcome.success
            << " ficheros=" << listing.info.fileCount << " carpetas=" << listing.info.dirCount
            << " total=" << listing.info.totalSize;
        if (!listing.info.comment.isEmpty())
            out << " comentario=" << QString::fromLatin1(listing.info.comment.toUtf8().toPercentEncoding());
        out << '\n';
        for (const ArchiveEntry &e : listing.entries) {
            out << (e.isDir() ? "D " : "F ") << e.size << ' ' << e.packedSize << ' ' << e.name
                << '\n';
        }
        exitCode = outcome.success ? 0 : 1;
        return true;
    }

    // --- Serie de volumenes --------------------------------------------------
    if (request.volumes) {
        const auto s = makeService();
        QString missing;
        const bool complete = s->volumesPresent(archive, &missing);
        out << "volumenes completos=" << complete
            << " falta=" << (missing.isEmpty() ? QStringLiteral("(nada)") : missing)
            << " primera=" << s->firstVolumeOf(archive) << '\n';
        exitCode = complete ? 0 : 1;
        return true;
    }

    // --- Probar / verificar --------------------------------------------------
    if (request.test) {
        const auto s = makeService();
        const ProcessOutcome o = s->test(archive, {}, request.password, !request.password.isEmpty());
        out << "probar diagnostico=" << int(o.diagnostic) << " exito=" << o.success << '\n';
        exitCode = o.success ? 0 : 1;
        return true;
    }

    // --- Crear ---------------------------------------------------------------
    if (!request.createPath.isEmpty()) {
        const auto s = makeService();
        CreateOptions o;
        o.archivePath = request.createPath;
        o.files = request.createItems;
        const ProcessOutcome r = s->create(o);
        out << "crear diagnostico=" << int(r.diagnostic) << " exito=" << r.success << '\n';
        exitCode = r.success ? 0 : 1;
        return true;
    }

    // --- Modificar: borrar y renombrar ---------------------------------------
    // --- Comentario del archivo ----------------------------------------------
    if (request.readComment) {
        const auto s = makeService();
        const QString comment = s->readComment(archive);
        out << "comentario=" << QString::fromLatin1(comment.toUtf8().toPercentEncoding()) << '\n';
        exitCode = 0;
        return true;
    }

    if (!request.commentFile.isEmpty()) {
        const auto s = makeService();
        const ProcessOutcome r = s->setComment(archive, request.commentFile);
        out << "comentario diagnostico=" << int(r.diagnostic) << " exito=" << r.success << '\n';
        exitCode = r.success ? 0 : 1;
        return true;
    }

    if (!request.removeItems.isEmpty() || !request.renameFrom.isEmpty()) {
        const auto s = makeService();
        ProcessOutcome r;
        QString what;
        if (!request.removeItems.isEmpty()) {
            r = s->removeItems(archive,
                               request.removeItems.split(QLatin1Char(','), Qt::SkipEmptyParts));
            what = QStringLiteral("borrar");
        } else {
            r = s->renameItem(archive, request.renameFrom, request.renameTo);
            what = QStringLiteral("renombrar");
        }
        out << what << " diagnostico=" << int(r.diagnostic) << " exito=" << r.success << '\n';
        if (!r.message.isEmpty())
            out << "  aviso=" << r.message << '\n';
        exitCode = r.success ? 0 : 1;
        return true;
    }

    // --- Extraer -------------------------------------------------------------
    // Lo usan tanto `--extract-to` como la sonda de pruebas.
    if (!request.extractTo.isEmpty()) {
        const auto s = makeService();
        ExtractOptions o;
        o.destination = request.extractTo;
        o.items = request.extractItems;
        if (!request.password.isEmpty()) {
            o.hasPassword = true;
            o.password = request.password;
        }
        const ProcessOutcome r = s->extract(archive, o);
        out << "extraer diagnostico=" << int(r.diagnostic) << " exito=" << r.success << '\n';
        exitCode = r.success ? 0 : 1;
        return true;
    }

    return false;
}

} // namespace qtrar
