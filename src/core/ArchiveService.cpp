#include "ArchiveService.h"

#include "ListParser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryDir>
#include <QRegularExpression>

namespace qtrar {

namespace {

const char *kInhibitConfiguration = "-cfg-";
// Evita que RAR ensucie su configuracion global del usuario.
const char *kNoConfigWrite = "-or-";

} // namespace

ArchiveService::ArchiveService(QObject *parent) : QObject(parent) {}

ArchiveFormat ArchiveService::detectFormat(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return ArchiveFormat::Unknown;

    const QByteArray head = f.read(16);
    f.close();

    // Firma RAR: "Rar!\x1a\x07\x00" (4.x) o "Rar!\x1a\x07\x01\x00" (5.x).
    // Un modulo SFX puede anteponer hasta 1 MB de datos, asi que la firma se
    // busca dentro de los primeros bytes leidos en vez de solo al inicio.
    static const QByteArray rar5 = QByteArray("Rar!\x1a\x07\x01\x00", 8);
    static const QByteArray rar4 = QByteArray("Rar!\x1a\x07\x00", 7);
    if (head.contains(rar5) || head.contains(rar4))
        return ArchiveFormat::Rar;

    if (head.startsWith("PK\x03\x04") || head.startsWith("PK\x05\x06")
        || head.startsWith("PK\x07\x08"))
        return ArchiveFormat::Zip;

    return ArchiveFormat::Unknown;
}

bool ArchiveService::isSupported(ArchiveFormat fmt)
{
    return fmt == ArchiveFormat::Rar || fmt == ArchiveFormat::Zip;
}

bool ArchiveService::isSupported(const QString &path)
{
    return isSupported(detectFormat(path));
}

ArchiveFormat ArchiveService::engineFor(const QString &archivePath) const
{
    return detectFormat(archivePath);
}

QStringList ArchiveService::buildListArgs(const QString &archivePath, const QString &password,
                                          bool hasPassword) const
{
    // -c-  : no mostrar comentarios
    // -y   : responder si a cualquier pregunta
    // -p   : nunca preguntar la clave (evita que la GUI se cuelgue)
    // -cfg-: ignorar la configuracion personal del usuario, para que el
    //        resultado sea reproducible entre maquinas.
    return {QStringLiteral("lt"), QString::fromLatin1(kInhibitConfiguration),
            QStringLiteral("-y"), QStringLiteral("-c-"),
            ProcessRunner::passwordSwitch(hasPassword, password), archivePath};
}

ArchiveListing ArchiveService::list(const QString &archivePath, const QString &password,
                                    bool hasPassword, ProcessOutcome *outcome) const
{
    ArchiveListing listing;

    if (!QFileInfo::exists(archivePath)) {
        if (outcome) {
            *outcome = {};
            outcome->diagnostic = Diagnostic::FileNotFound;
            outcome->success = false;
            outcome->message = archivePath;
        }
        return listing;
    }

    const ArchiveFormat fmt = engineFor(archivePath);

    if (fmt == ArchiveFormat::Zip) {
        // unrar NO sabe leer ZIP (comprobado: "is not RAR archive" con exit 0).
        if (m_tools.sevenZipPath.isEmpty()) {
            if (outcome) {
                *outcome = {};
                outcome->diagnostic = Diagnostic::OpenError;
                outcome->message = QStringLiteral("7z no esta disponible: no se pueden abrir archivos ZIP");
            }
            return listing;
        }
        // Sin -bso0/-bse0: p7zip los tolera, pero el 7-Zip oficial corta
        // de verdad la salida estandar y el listado `-slt` llegaria vacio.
        const RunResult r = m_runner.run(m_tools.sevenZipPath,
                                         {QStringLiteral("l"), QStringLiteral("-slt"),
                                          QStringLiteral("-y"), archivePath},
                                         QString(), 0);
        const ProcessOutcome po = Diagnostics::analyze(r, true);
        if (outcome)
            *outcome = po;
        if (!r.stdOut.contains(QLatin1String("Path = ")))
            return listing;
        return ListParser::parseSevenZip(r.stdOut, archivePath);
    }

    if (fmt != ArchiveFormat::Rar || m_tools.unrarPath.isEmpty()) {
        if (outcome) {
            *outcome = {};
            outcome->diagnostic = fmt == ArchiveFormat::Rar
                ? Diagnostic::OpenError
                : Diagnostic::NotAnArchive;
            outcome->message = fmt == ArchiveFormat::Rar
                ? QStringLiteral("unrar no esta disponible")
                : QString();
        }
        return listing;
    }

    const RunResult r = m_runner.run(m_tools.unrarPath, buildListArgs(archivePath, password, hasPassword),
                                     QString(), 30000);
    const ProcessOutcome po = Diagnostics::analyze(r, true);
    if (outcome)
        *outcome = po;

    if (!po.success && !po.needsPassword())
        return listing;

    listing = ListParser::parseUnrarTechnical(r.stdOut, archivePath);
    return listing;
}

QStringList ArchiveService::listDirectory(const QString &archivePath, const QString &subPath,
                                          const QString &password, bool hasPassword,
                                          ProcessOutcome *outcome) const
{
    if (const ArchiveFormat fmt = engineFor(archivePath); fmt == ArchiveFormat::Zip) {
        // Para ZIP se recorta sobre la lista ya parseada; `7z` no tiene un
        // listado "desnudo" por subruta comparable a `unrar lb`.
        if (outcome) {
            *outcome = {};
            outcome->success = true;
        }
        const ArchiveListing all = list(archivePath);
        return all.entries.isEmpty() ? QStringList()
                                     : all.childrenOf(subPath);
    }

    QStringList args = {QStringLiteral("lb"), QString::fromLatin1(kInhibitConfiguration),
                        QStringLiteral("-y"), QStringLiteral("-c-"),
                        ProcessRunner::passwordSwitch(hasPassword, password), archivePath};
    if (!subPath.isEmpty()) {
        QString prefix = subPath;
        if (!prefix.endsWith(u'/'))
            prefix += u'/';
        args << prefix + QStringLiteral("*");
    }

    const RunResult r = m_runner.run(m_tools.unrarPath, args, QString(), 30000);
    const ProcessOutcome po = Diagnostics::analyze(r, true);
    if (outcome)
        *outcome = po;
    if (!po.success)
        return {};
    return ListParser::parseBareList(r.stdOut);
}

ProcessOutcome ArchiveService::test(const QString &archivePath, const QStringList &items,
                                    const QString &password, bool hasPassword) const
{
    if (engineFor(archivePath) == ArchiveFormat::Zip) {
        if (m_tools.sevenZipPath.isEmpty()) {
            ProcessOutcome po;
            po.diagnostic = Diagnostic::OpenError;
            po.message = QStringLiteral("7z no esta disponible");
            return po;
        }
        QStringList args = {QStringLiteral("t"), QStringLiteral("-y"), archivePath};
        args << items;
        const RunResult r = m_runner.run(m_tools.sevenZipPath, args, QString(), 0);
        if (!r.ok()) {
            return Diagnostics::analyze(r, true);
        }
        ProcessOutcome ok;
        ok.success = true;
        return ok;
    }

    QStringList args = {QStringLiteral("t"), QString::fromLatin1(kInhibitConfiguration),
                        QStringLiteral("-y"), QStringLiteral("-c-"),
                        ProcessRunner::passwordSwitch(hasPassword, password), archivePath};
    args << items;
    const RunResult r = m_runner.run(m_tools.unrarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::extract(const QString &archivePath,
                                       const ExtractOptions &options) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (options.destination.isEmpty()) {
        fail.diagnostic = Diagnostic::OpenError;
        fail.message = QStringLiteral("No se ha indicado la carpeta de destino");
        return fail;
    }
    if (!QDir().mkpath(options.destination)) {
        fail.diagnostic = Diagnostic::WriteError;
        fail.message = options.destination;
        return fail;
    }

    // Sin -y unrar podria quedarse esperando confirmaciones de sobrescritura.
    QStringList common = {QString::fromLatin1(kInhibitConfiguration), QStringLiteral("-y"),
                          QStringLiteral("-c-"),
                          ProcessRunner::passwordSwitch(options.hasPassword, options.password)};

    RunResult r;
    if (engineFor(archivePath) == ArchiveFormat::Zip) {
        if (m_tools.sevenZipPath.isEmpty()) {
            fail.diagnostic = Diagnostic::OpenError;
            fail.message = QStringLiteral("7z no esta disponible");
            return fail;
        }
        // 7z no tiene equivalente a -e; "e" es sin rutas, "x" con rutas.
        QStringList args = {options.extractToStdout ? QStringLiteral("e") : QStringLiteral("x"),
                            QStringLiteral("-bso0"), QStringLiteral("-bse0"), QStringLiteral("-y")};
        switch (options.existingFiles) {
        // MainWindow resuelve Confirm con un dialogo previo; si un llamador
        // del nucleo lo pasa directamente, el comportamiento seguro es omitir.
        case ExistingFilesMode::Confirm:
        case ExistingFilesMode::Skip: args << QStringLiteral("-aos"); break;
        case ExistingFilesMode::Overwrite: args << QStringLiteral("-aoa"); break;
        case ExistingFilesMode::Rename: args << QStringLiteral("-aou"); break;
        }
        args << QStringLiteral("-o") + options.destination << archivePath;
        if (options.hasPassword)
            args << QStringLiteral("-p") + options.password;
        args << options.items;
        r = m_runner.run(m_tools.sevenZipPath, args, QString(), 0);
    } else {
        if (m_tools.unrarPath.isEmpty()) {
            fail.diagnostic = Diagnostic::OpenError;
            fail.message = QStringLiteral("unrar no esta disponible");
            return fail;
        }
        QStringList args = {options.extractToStdout ? QStringLiteral("e") : QStringLiteral("x")};
        args << common;
        switch (options.updateMode) {
        case ExtractUpdateMode::Replace: break;
        case ExtractUpdateMode::Update: args << QStringLiteral("-u"); break;
        case ExtractUpdateMode::Freshen: args << QStringLiteral("-f"); break;
        }
        if (options.keepBrokenFiles)
            args << QStringLiteral("-kb");
        switch (options.existingFiles) {
        case ExistingFilesMode::Confirm:
        case ExistingFilesMode::Skip: args << QStringLiteral("-o-"); break;
        case ExistingFilesMode::Overwrite: args << QStringLiteral("-o+"); break;
        case ExistingFilesMode::Rename: args << QStringLiteral("-or"); break;
        }
        if (!options.preservePaths)
            args << QStringLiteral("-ep");
        // El destino va con -op y no como argumento suelto: verified que un
        // "ruta/" al final solo se desambigua por la barra final, mientras que
        // -op compone bien con las mascaras de fichero.
        args << QStringLiteral("-op") + QDir(options.destination).absolutePath();
        args << archivePath;
        args << options.items;
        r = m_runner.run(m_tools.unrarPath, args, QString(), 0);
    }

    return Diagnostics::analyze(r, false);
}

namespace {

QStringList rarCreateSwitches(const CreateOptions &o)
{
    QStringList a;
    a << QString::fromLatin1(kInhibitConfiguration) << QStringLiteral("-y");

    // Nivel de compresion. -m0 guarda sin comprimir, como "Almacenar" en WinRAR.
    a << QStringLiteral("-m") + QString::number(o.storeOnly ? 0 : qBound(0, o.compressionLevel, 5));
    if (!o.storeOnly && o.dictionarySizeMb > 0)
        a << QStringLiteral("-md") + QString::number(o.dictionarySizeMb);

    if (o.solid)
        a << QStringLiteral("-s");
    if (o.hasPassword)
        a << (o.encryptHeaders ? QStringLiteral("-hp") : QStringLiteral("-p")) + o.password;
    if (o.createVolumes && !o.volumeSize.isEmpty())
        a << QStringLiteral("-v") + o.volumeSize;
    if (o.sfx)
        a << QStringLiteral("-sfx");
    if (o.addRecoveryRecord)
        a << QStringLiteral("-rr");

    a << QStringLiteral("-c-");
    return a;
}

} // namespace

ProcessOutcome ArchiveService::create(const CreateOptions &options) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }
    if (options.files.isEmpty()) {
        fail.diagnostic = Diagnostic::NoFilesMatched;
        return fail;
    }

    const QFileInfo fi(options.archivePath);
    const bool exists = fi.exists();

    QStringList args;
    if (exists && options.updateExisting)
        args << QStringLiteral("u");   // anadir/actualizar
    else
        args << QStringLiteral("a");   // crear

    args << rarCreateSwitches(options);
    args << options.archivePath;
    args << options.files;

    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::createZip(const CreateOptions &options) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.sevenZipPath.isEmpty()) {
        fail.diagnostic = Diagnostic::OpenError;
        fail.message = QStringLiteral("7z no esta disponible");
        return fail;
    }
    if (options.files.isEmpty()) {
        fail.diagnostic = Diagnostic::NoFilesMatched;
        return fail;
    }

    // 7z guarda la jerarquia; -spf desactiva el reemplazo de la ruta de
    // trabajo por el nombre de la carpeta de primer nivel.
    QStringList args = {QStringLiteral("a"), QStringLiteral("-tzip"), QStringLiteral("-y"),
                        QStringLiteral("-bso0"), QStringLiteral("-bse0"),
                        QStringLiteral("-mx") + QString::number(qBound(0, options.compressionLevel, 9))};
    if (options.hasPassword)
        args << QStringLiteral("-p") + options.password;
    args << options.archivePath;
    args << options.files;

    const RunResult r = m_runner.run(m_tools.sevenZipPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::removeItems(const QString &archivePath,
                                           const QStringList &items) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }
    if (items.isEmpty()) {
        fail.diagnostic = Diagnostic::NoFilesMatched;
        return fail;
    }

    QStringList args = {QStringLiteral("d"), QString::fromLatin1(kInhibitConfiguration),
                        QStringLiteral("-y"), QStringLiteral("-c-"), archivePath};
    args << items;
    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

QString ArchiveService::readComment(const QString &archivePath) const
{
    if (m_tools.unrarPath.isEmpty() || engineFor(archivePath) != ArchiveFormat::Rar)
        return {};

    // Sin `-c-` a proposito: es lo unico que hace que unrar muestre el texto.
    const QStringList args = {QStringLiteral("lt"), QString::fromLatin1(kInhibitConfiguration),
                              QStringLiteral("-y"), archivePath};
    const RunResult r = m_runner.run(m_tools.unrarPath, args, QString(), 30000);
    if (!r.stdOut.contains(QLatin1String("Archive comment:")))
        return {};
    return ListParser::parseUnrarTechnical(r.stdOut, archivePath).info.comment;
}

ProcessOutcome ArchiveService::setComment(const QString &archivePath,
                                         const QString &commentFile) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }
    if (!QFileInfo::exists(commentFile)) {
        fail.diagnostic = Diagnostic::FileNotFound;
        fail.message = QStringLiteral("No existe el fichero de comentario: %1").arg(commentFile);
        return fail;
    }

    // Comprobado con RAR 7.23: `-z` NO admite el nombre del fichero en el
    // argumento siguiente. Con `-z fichero.txt` rar toma "fichero.txt" como el
    // nombre del archivo y responde "Bad archive", que parece un fallo pero es
    // otra cosa. El nombre va pegado al interruptor.
    const QStringList args = {QStringLiteral("c"),
                              QStringLiteral("-z") + QFileInfo(commentFile).absoluteFilePath(),
                              QString::fromLatin1(kInhibitConfiguration), QStringLiteral("-y"),
                              QString::fromLatin1(kNoConfigWrite), archivePath};
    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::renameItem(const QString &archivePath, const QString &from,
                                          const QString &to) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }
    if (from.isEmpty() || to.isEmpty()) {
        fail.diagnostic = Diagnostic::NoFilesMatched;
        return fail;
    }

    // Comprobado con RAR 7.23: el comando de renombrar es `rn`. `rr` NO vale
    // (es el registro de recuperacion: responde "Done" sin renombrar nada, que
    // es peor que un fallo), y `unrar rn` no existe. Asi que solo `rar`.
    const QStringList args = {QStringLiteral("rn"), QString::fromLatin1(kInhibitConfiguration),
                              QStringLiteral("-y"), QStringLiteral("-c-"), archivePath, from, to};
    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::addRecoveryRecord(const QString &archivePath) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }

    // `rr` es el registro de recuperacion. A diferencia de `rn` (que si
    // renombra), `rr` responde "Done" cuando de verdad ha hecho su trabajo, y
    // solo existe en `rar`.
    const QStringList args = {QStringLiteral("rr"), QString::fromLatin1(kInhibitConfiguration),
                              QStringLiteral("-y"), archivePath};
    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::recoverArchive(const QString &archivePath) const
{
    ProcessOutcome fail;
    fail.success = false;

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }

    // `rc` reconstruye el archivo a partir del registro de recuperacion. Como
    // `rr`, solo existe en `rar` y devuelve 0 aunque no haya nada que rehacer,
    // asi que no se puede usar el codigo de salida para decir "reparado".
    const QStringList args = {QStringLiteral("rc"), QString::fromLatin1(kInhibitConfiguration),
                              QStringLiteral("-y"), archivePath};
    const RunResult r = m_runner.run(m_tools.rarPath, args, QString(), 0);
    return Diagnostics::analyze(r, false);
}

ProcessOutcome ArchiveService::encryptFileNames(const QString &archivePath, const QString &password,
                                                const QString &items, QString *stagedPath) const
{
    ProcessOutcome fail;
    fail.success = false;
    if (stagedPath)
        stagedPath->clear();

    if (m_tools.rarPath.isEmpty()) {
        fail.diagnostic = Diagnostic::ToolNotFound;
        fail.message = QStringLiteral("El binario 'rar' no esta disponible");
        return fail;
    }
    if (password.isEmpty()) {
        fail.diagnostic = Diagnostic::WrongPassword;
        fail.message = QStringLiteral("Hace falta una contrasena para cifrar los nombres");
        return fail;
    }

    // No hay forma de poner -hp sobre un archivo que ya existe: hay que
    // deshacerlo y rehacerlo. Se trabaja en un temporal y el original solo se
    // toca cuando el nuevo esta listo, para no quedarse sin archivo si algo
    // falla por el camino.
    QTemporaryDir work;
    if (!work.isValid()) {
        fail.diagnostic = Diagnostic::OpenError;
        fail.message = QStringLiteral("No se pudo crear la carpeta temporal");
        return fail;
    }

    ExtractOptions ex;
    ex.destination = work.path();
    if (!items.isEmpty())
        ex.items = items.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    const ProcessOutcome extracted = extract(archivePath, ex);
    if (!extracted.success)
        return extracted;

    // El temporal se borra al salir de la funcion, asi que el archivo nuevo se
    // deja junto al original y lo mueve la interfaz cuando todo ha ido bien.
    const QFileInfo fi(archivePath);
    const QString staged = fi.absolutePath() + QStringLiteral("/.qtrar-cifrado-")
                           + fi.fileName();

    // Se pasan rutas absolutas y no un "*": el proceso no cambia de carpeta de
    // trabajo, asi que un glob relativo se resolveria contra el directorio desde
    // el que se lanzo QtRAR y se empaquetaria otra cosa (o nada).
    QStringList contents;
    const QStringList entries = QDir(work.path()).entryList(QDir::AllEntries | QDir::NoDotAndDotDot);
    for (const QString &entry : entries)
        contents.append(QDir(work.path()).absoluteFilePath(entry));
    if (contents.isEmpty()) {
        fail.diagnostic = Diagnostic::NoFilesMatched;
        fail.message = QStringLiteral("El archivo esta vacio: no hay nada que reempaquetar");
        return fail;
    }

    CreateOptions co;
    co.archivePath = staged;
    co.files = contents;
    co.hasPassword = true;
    co.password = password;
    co.encryptHeaders = true;   // -hp: tambien los nombres
    co.updateExisting = false;  // -a: el archivo es nuevo
    const ProcessOutcome created = create(co);
    if (!created.success)
        return created;

    if (stagedPath)
        *stagedPath = staged;
    return {};
}

bool ArchiveService::volumesPresent(const QString &archivePath, QString *missingVolume) const
{
    if (missingVolume)
        missingVolume->clear();

    const QFileInfo fi(archivePath);
    if (!fi.exists()) {
        if (missingVolume)
            *missingVolume = archivePath;
        return false;
    }

    // Solo tiene sentido en un archivo que forme parte de una serie. Un RAR5
    // multivolume se nombra `<base>.part<N>.rar` (rar 7.23 no pone el cero a
    // la izquierda) y uno RAR4 `<base>.rar` + `<base>.rNN`.
    static const QRegularExpression partRe(
        QStringLiteral("\\.part\\d+\\.rar$"), QRegularExpression::CaseInsensitiveOption);
    const bool looksLikeFirstVolume =
        partRe.match(fi.fileName()).hasMatch()
        || fi.fileName().compare(QStringLiteral("rar"), Qt::CaseInsensitive) == 0;
    if (!looksLikeFirstVolume)
        return true;

    // El sistema de archivos no permite distinguir "no hay mas volumenes" de
    // "falta el volumen 3": solo unrar sabe donde acaba la serie. Se le
    // pregunta con una prueba, que es la unica forma fiable.
    if (m_tools.unrarPath.isEmpty()) {
        if (missingVolume)
            *missingVolume = archivePath;
        return false;
    }

    const QStringList args = {QStringLiteral("t"), QString::fromLatin1(kInhibitConfiguration),
                              QStringLiteral("-y"), QStringLiteral("-c-"),
                              ProcessRunner::passwordSwitch(false, {}), archivePath};
    const RunResult r = m_runner.run(m_tools.unrarPath, args, QString(), 0);
    static const QRegularExpression missingRe(
        QStringLiteral("Cannot find volume\\s+(\\S+)"),
        QRegularExpression::CaseInsensitiveOption);
    if (const QRegularExpressionMatch m = missingRe.match(r.stdErr + QLatin1Char('\n') + r.stdOut);
        m.hasMatch()) {
        if (missingVolume) {
            const QString vol = m.captured(1);
            *missingVolume = QDir::isAbsolutePath(vol) ? vol : fi.absolutePath() + QLatin1Char('/') + vol;
        }
        return false;
    }

    return true;
}

QString ArchiveService::firstVolumeOf(const QString &archivePath) const
{
    // Si se abre `multi.part3.rar` conviene subir al `multi.part1.rar`, que es
    // por donde se abre una serie, igual que hace WinRAR.
    const QFileInfo fi(archivePath);
    const QString dir = fi.absolutePath();
    const QString name = fi.fileName();
    static const QRegularExpression partRe(
        QStringLiteral("^(.*)\\.part(\\d+)\\.rar$"), QRegularExpression::CaseInsensitiveOption);
    if (!partRe.match(name).hasMatch())
        return archivePath;

    const QString base = partRe.match(name).captured(1);
    for (int n = 1;; ++n) {
        QString candidate = QStringLiteral("%1.part%2.rar").arg(base).arg(n);
        if (QFileInfo::exists(dir + QLatin1Char('/') + candidate))
            return dir + QLatin1Char('/') + candidate;
        candidate = QStringLiteral("%1.part%2.rar").arg(base).arg(n, 2, 10, QLatin1Char('0'));
        if (QFileInfo::exists(dir + QLatin1Char('/') + candidate))
            return dir + QLatin1Char('/') + candidate;
        if (n > 10000)
            break;
    }
    return archivePath;
}

} // namespace qtrar
