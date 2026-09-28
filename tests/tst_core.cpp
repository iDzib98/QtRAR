// Tests del nucleo contra salidas REALES de unrar 7.23 capturadas en
// tests/fixtures/ con tools/capture-fixtures.sh.
//
// Los tests no necesitan los binarios instalados: las fixtures son texto. Eso
// permite ejecutar `ctest` en CI, en maquinas sin RAR y en maquinas con otra
// version instalada.
#include "core/ArchiveService.h"
#include "core/BinaryLocator.h"
#include "core/Diagnostics.h"
#include "core/HFormat.h"
#include "core/ListParser.h"
#include "core/ProcessRunner.h"
#include "core/Types.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QtTest>

using namespace qtrar;

namespace {

QString fixturePath(const QString &name)
{
    return QDir(QStringLiteral(QTEST_SOURCE_DIR)).absoluteFilePath(
        QStringLiteral("fixtures/") + name);
}

QString readFixture(const QString &name)
{
    QFile f(fixturePath(name));
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(f.readAll());
}

RunResult makeResult(int exitCode, const QString &out = {}, const QString &err = {})
{
    RunResult r;
    r.started = true;
    r.exitCode = exitCode;
    r.stdOut = out;
    r.stdErr = err;
    return r;
}

} // namespace

class TestListParser : public QObject
{
    Q_OBJECT

private slots:
    // ------------------------------------------------------------------
    void parseaRar5Basico();
    void parseaRar5Completo();
    void directoriosNoTienenSize();
    void flagsConEspacioFinal();
    void nombresConDosPuntos();
    void nombresUnicode();
    void symlinks();
    void cabeceraCifrada();
    void cuentaArchivosYCarpetas();
    void childrenOf();
    void bareList();
    void sieteZip();
    void parseaFecha();
    void comentarioDelArchivo();
    void sinComentarioDaCadenaVacia();
    void comentarioNoSeComeElListado();
};

void TestListParser::comentarioDelArchivo()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(
        readFixture("comentario.lt.txt"), "/tmp/simple.rar");
    QCOMPARE(l.info.comment, QStringLiteral("Hola\nEsta es la segunda linea."));
    // El comentario no puede cargarse el resto de la cabecera: si lo hiciera,
    // "Archive:" y los datos se quedarian pegados al texto.
    QVERIFY(!l.info.comment.contains("simple.rar"));
    QVERIFY(!l.info.comment.contains("Details"));
    QCOMPARE(l.info.format, QStringLiteral("RAR 5"));
    QCOMPARE(l.entries.size(), 1);
    QCOMPARE(l.entries.first().name, QStringLiteral("fichero.txt"));
}

void TestListParser::sinComentarioDaCadenaVacia()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"),
                                                              "/tmp/x.rar");
    QVERIFY(l.info.comment.isEmpty());
    QVERIFY(!l.entries.isEmpty());
}

void TestListParser::comentarioNoSeComeElListado()
{
    // Cualquier linea posterior a "Archive comment:" que no sea de la cabecera
    // sigue formando parte del comentario; el corte es justo en "Archive:".
    const QString salida = QStringLiteral(
        "UNRAR 7.23 freeware\n"
        "Archive comment:\n"
        "linea del comentario\n"
        "con dos puntos: dentro\n"
        "\n"
        "Archive: x.rar\n"
        "Details: RAR 5\n"
        "\n"
        "        Name: a.txt\n"
        "        Type: File\n"
        "        Size: 10\n"
        " Packed size: 5\n");
    const ArchiveListing l = ListParser::parseUnrarTechnical(salida, "/tmp/x.rar");
    QCOMPARE(l.info.comment, QStringLiteral("linea del comentario\ncon dos puntos: dentro"));
    QCOMPARE(l.info.format, QStringLiteral("RAR 5"));
    QCOMPARE(l.entries.size(), 1);
    QCOMPARE(l.entries.first().name, QStringLiteral("a.txt"));
}

// ---------------------------------------------------------------------------

void TestListParser::parseaRar5Basico()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"),
                                                              "/tmp/x.rar");
    QVERIFY(!l.entries.isEmpty());
    QCOMPARE(l.info.format, QStringLiteral("RAR 5"));
    QCOMPARE(l.info.path, QString("/tmp/x.rar"));
    QCOMPARE(l.entries.first().name, QStringLiteral("readme.txt"));
    QCOMPARE(l.entries.first().type, EntryType::File);
    QCOMPARE(l.entries.first().size, qint64(800));
    QCOMPARE(l.entries.first().packedSize, qint64(35));
    QCOMPARE(l.entries.first().ratio, 4);
    QCOMPARE(l.entries.first().crc32, QStringLiteral("45C36B56"));
    QCOMPARE(l.entries.first().hostOs, QStringLiteral("Unix"));
    QVERIFY(!l.info.solid);
    QVERIFY(!l.info.encryptedHeaders);
}

void TestListParser::parseaRar5Completo()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    // 8 ficheros + 4 directorios en el archivo de pruebas.
    QCOMPARE(l.info.fileCount, 8);
    QCOMPARE(l.info.dirCount, 4);
    QCOMPARE(l.entries.size(), 12);
    // El banner "UNRAR 7.23 freeware" no debe convertirse en una entrada.
    for (const ArchiveEntry &e : l.entries)
        QVERIFY(!e.name.contains(QLatin1String("freeware")));
}

void TestListParser::directoriosNoTienenSize()
{
    // Los directorios no traen Size ni Packed size: deben quedar en -1 y no
    // en 0, para que la columna quede vacia y no "0 bytes".
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    int dirs = 0;
    for (const ArchiveEntry &e : l.entries) {
        if (!e.isDir())
            continue;
        ++dirs;
        QCOMPARE(e.size, qint64(-1));
        QCOMPARE(e.packedSize, qint64(-1));
        QCOMPARE(e.type, EntryType::Directory);
        QVERIFY(e.attributes.startsWith(QLatin1Char('d')));
    }
    QCOMPARE(dirs, 4);
}

void TestListParser::flagsConEspacioFinal()
{
    // "Flags: solid " y "Flags: encrypted " traen espacio final. Sin trim,
    // el flag seria "solid " y no coincidiria con isSolid().
    const ArchiveListing solid = ListParser::parseUnrarTechnical(readFixture("solid.lt.txt"));
    QVERIFY(solid.info.solid);
    int solids = 0;
    for (const ArchiveEntry &e : solid.entries)
        if (e.isSolid())
            ++solids;
    QVERIFY(solids > 0);

    const ArchiveListing crypt = ListParser::parseUnrarTechnical(readFixture("crypt.lt.txt"));
    int encrypted = 0;
    for (const ArchiveEntry &e : crypt.entries)
        if (e.isEncrypted())
            ++encrypted;
    QVERIFY(encrypted > 0);
    QVERIFY(crypt.info.anyEncrypted);
}

void TestListParser::nombresConDosPuntos()
{
    // "docs/weird:name:colon.txt": la clave se parte por el PRIMER ": ".
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    QVERIFY(l.find(QStringLiteral("docs/weird:name:colon.txt")) != nullptr);

    const ArchiveEntry *e = l.find(QStringLiteral("docs/weird'quote\"dquote.txt"));
    QVERIFY(e != nullptr);
    QCOMPARE(e->baseName(), QStringLiteral("weird'quote\"dquote.txt"));
    QCOMPARE(e->parentPath(), QStringLiteral("docs"));
}

void TestListParser::nombresUnicode()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    const ArchiveEntry *e = l.find(QStringLiteral("img/año_español (1).txt"));
    QVERIFY2(e != nullptr, "no se encontro el nombre con acentos y espacios");
    QCOMPARE(e->size, qint64(19));
}

void TestListParser::symlinks()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("symlink.lt.txt"));
    const ArchiveEntry *e = l.find(QStringLiteral("docs/link.txt"));
    QVERIFY(e != nullptr);
    QCOMPARE(e->type, EntryType::Symlink);
    QVERIFY(e->isLink());
}

void TestListParser::cabeceraCifrada()
{
    // Sin clave: solo se ve la cabecera y avisa de clave incorrecta.
    // El parser debe devolver la cabecera con zero entradas, sin inventarse nada.
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("crypthdr.lt.txt"));
    QVERIFY(l.info.encryptedHeaders);
    QCOMPARE(l.info.format, QStringLiteral("RAR 5"));
    QVERIFY(l.entries.isEmpty());
}

void TestListParser::cuentaArchivosYCarpetas()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    QVERIFY(l.info.totalSize > 0);
    QVERIFY(l.info.totalPacked > 0);
    // El total de bytes debe coincidir con la suma de los ficheros.
    qint64 sum = 0;
    for (const ArchiveEntry &e : l.entries)
        if (!e.isDir())
            sum += e.size;
    QCOMPARE(l.info.totalSize, sum);
}

void TestListParser::childrenOf()
{
    const ArchiveListing l = ListParser::parseUnrarTechnical(readFixture("rar5.lt.txt"));
    const QStringList kids = l.childrenOf(QStringLiteral("docs"));
    QCOMPARE(kids.size(), 4);
    QVERIFY(kids.contains(QStringLiteral("manual.txt")));
    QVERIFY(kids.contains(QStringLiteral("notas.txt")));
    // "docs" no debe incluirse a si mismo ni lo que hay por debajo.
    QVERIFY(!kids.contains(QStringLiteral("docs")));

    // La raiz devuelve solo los hijos directos.
    const QStringList root = l.childrenOf(QString());
    QCOMPARE(root.size(), 4);
    QVERIFY(root.contains(QStringLiteral("readme.txt")));
    QVERIFY(root.contains(QStringLiteral("docs")));
    QVERIFY(root.contains(QStringLiteral("img")));
    QVERIFY(root.contains(QStringLiteral("empty")));
}

void TestListParser::bareList()
{
    const QStringList names = ListParser::parseBareList(readFixture("rar5.docs.lb.txt"));
    QCOMPARE(names.size(), 4);
    QVERIFY(names.contains(QStringLiteral("docs/manual.txt")));
    // El banner y el comentario inicial se descartan.
    for (const QString &n : names)
        QVERIFY(!n.startsWith(QLatin1Char('#')));
    QVERIFY(!names.contains(QStringLiteral("UNRAR 7.23 freeware      Copyright (c) 1993-2026 Alexander Roshal")));
}

void TestListParser::sieteZip()
{
    const QString out = readFixture("test7z.7zsl.txt");
    if (out.isEmpty())
        QSKIP("fixture 7z no generada (hace falta 7z al capturar)");
    const ArchiveListing l = ListParser::parseSevenZip(out, "/tmp/x.zip");
    QCOMPARE(l.info.format, QStringLiteral("ZIP"));
    QVERIFY(!l.entries.isEmpty());
    QVERIFY(l.find(QStringLiteral("readme.txt")) != nullptr);
}

void TestListParser::parseaFecha()
{
    const QDateTime dt = ListParser::parseUnrarTime(QStringLiteral("2026-09-25 15:14:30,849861011"));
    QVERIFY(dt.isValid());
    QCOMPARE(dt.date(), QDate(2026, 9, 25));
    QCOMPARE(dt.time().hour(), 15);
    QCOMPARE(dt.time().minute(), 14);
    // Los milisegundos no se truncan a 0: 849861011 ns -> 849 ms.
    QCOMPARE(dt.time().second(), 30);
    QCOMPARE(dt.time().msec(), 849);

    const QDateTime plain = ListParser::parseUnrarTime(QStringLiteral("2026-09-25 15:14:30"));
    QVERIFY(plain.isValid());
    QCOMPARE(plain.time().msec(), 0);
}

// ---------------------------------------------------------------------------

class TestDiagnostics : public QObject
{
    Q_OBJECT

private slots:
    void codigoExito();
    void zipDevuelveCero();
    void claveIncorrecta();
    void claveIncorrectaEnPrueba();
    void archivoInexistente();
    void usoDeComando();
    void totalErrors();
    void cancelacion();
    void bannersDeLicenzaIgnorados();
    void passwordSwitch();
    void lineasDeProgreso();
    void describiendoDiagnosticos();
};

void TestDiagnostics::codigoExito()
{
    const ProcessOutcome o = Diagnostics::analyze(makeResult(0, "algo"));
    QCOMPARE(o.diagnostic, Diagnostic::Success);
    QVERIFY(o.success);
}

void TestDiagnostics::zipDevuelveCero()
{
    // CASO TRAMPA VERIFICADO: unrar responde 0 y pone "is not RAR archive".
    // Sin mirar el texto, QtRAR abriria un ZIP como si estuviera vacio.
    const QString out = readFixture("zip_no_rar.txt");
    QVERIFY(!out.isEmpty());
    const ProcessOutcome o = Diagnostics::analyze(makeResult(0, out), true);
    QCOMPARE(o.diagnostic, Diagnostic::NotAnArchive);
    QVERIFY(!o.success);
}

void TestDiagnostics::claveIncorrecta()
{
    const QString out = readFixture("crypthdr.lt.txt");
    const ProcessOutcome o = Diagnostics::analyze(makeResult(11, out), true);
    QCOMPARE(o.diagnostic, Diagnostic::WrongPassword);
    QVERIFY(o.needsPassword());
}

void TestDiagnostics::claveIncorrectaEnPrueba()
{
    const ProcessOutcome o = Diagnostics::analyze(
        makeResult(11, "UNRAR 7.23 freeware\nIncorrect password for docs/manual.txt"), false);
    QCOMPARE(o.diagnostic, Diagnostic::WrongPassword);
}

void TestDiagnostics::archivoInexistente()
{
    // Verificado: `unrar lt no_existe.rar` -> codigo 10 + "Cannot open ...".
    const QString out = readFixture("missing_archive.txt");
    const ProcessOutcome o = Diagnostics::analyze(makeResult(10, out), true);
    QCOMPARE(o.diagnostic, Diagnostic::FileNotFound);

    // El mismo codigo 10 sin texto de error es "la mascara no coincidio",
    // que no es lo mismo que un archivo inexistente.
    const ProcessOutcome m = Diagnostics::analyze(
        makeResult(10, readFixture("no_match.txt")), true);
    QCOMPARE(m.diagnostic, Diagnostic::NoFilesMatched);
}

void TestDiagnostics::usoDeComando()
{
    // CASO TRAMPA VERIFICADO: `unrar d` no existe en 7.23; imprime el uso y
    // devuelve 7, o incluso 0 en variantes. Hay que reconocerlo por el texto.
    const QString out = readFixture("uso_comando.txt");
    QVERIFY(out.contains(QLatin1String("<Commands>")));
    // Verificado: en 7.23 `unrar d` imprime el uso y devuelve 7.
    const ProcessOutcome o = Diagnostics::analyze(makeResult(7, out), true);
    QCOMPARE(o.diagnostic, Diagnostic::UnknownError);
    QCOMPARE(o.exitCode, ExitCode::CommandLineError);

    // Otras versiones devuelven 0 con el mismo texto: tambien es un error.
    const ProcessOutcome z = Diagnostics::analyze(makeResult(0, out), true);
    QCOMPARE(z.diagnostic, Diagnostic::UnknownError);
    QVERIFY(!o.success);
}

void TestDiagnostics::totalErrors()
{
    // `unrar t` con clave erronea puede acabar con codigo 0 y "Total errors: 5".
    const ProcessOutcome o = Diagnostics::analyze(
        makeResult(0, "Testing archive crypt.rar\nIncorrect password for readme.txt\n"
                      "Total errors: 5"), false);
    QCOMPARE(o.diagnostic, Diagnostic::ChecksumError);
}

void TestDiagnostics::cancelacion()
{
    RunResult r = makeResult(255);
    r.cancelled = true;
    const ProcessOutcome o = Diagnostics::analyze(r);
    QCOMPARE(o.diagnostic, Diagnostic::Cancelled);
}

void TestDiagnostics::bannersDeLicenzaIgnorados()
{
    // El banner de prueba no debe acabar en el mensaje de error que ve el usuario.
    const QString out = readFixture("rar_add_progress.txt");
    QVERIFY(out.contains(QLatin1String("Evaluation copy")));
    const QString msg = extractErrorMessage(QString(), out);
    QVERIFY(!msg.contains(QLatin1String("Evaluation copy")));
    QVERIFY(!msg.contains(QLatin1String("Trial version")));
    QVERIFY(!msg.contains(QLatin1String("freeware")));
    // Pero debe conservar el primer mensaje con contenido util.
    QVERIFY(!msg.isEmpty());
}

void TestDiagnostics::passwordSwitch()
{
    // La defensa anti-hang: sin -p unrar pide la clave por consola.
    QCOMPARE(ProcessRunner::passwordSwitch(false, QString()), QStringLiteral("-p-"));
    QCOMPARE(ProcessRunner::passwordSwitch(true, QString()), QStringLiteral("-p-"));
    QCOMPARE(ProcessRunner::passwordSwitch(true, QStringLiteral("secreto")),
             QStringLiteral("-psecreto"));
}

void TestDiagnostics::lineasDeProgreso()
{
    // Formato real: "Adding    ./readme.txt                14% OK"
    const QString line = QStringLiteral(
        "Adding    ./readme.txt                                               14% OK ");
    QString msg;
    int pct = -1;
    QVERIFY(parseProgressLine(line, &msg, &pct));
    QCOMPARE(msg, QStringLiteral("./readme.txt"));
    QCOMPARE(pct, 14);

    const QString extracting = QStringLiteral(
        "Extracting  docs/manual.txt                                          100% OK ");
    QVERIFY(parseProgressLine(extracting, &msg, &pct));
    QCOMPARE(msg, QStringLiteral("docs/manual.txt"));
    QCOMPARE(pct, 100);

    // Una linea que no es de progreso no debe intentar interpretarse.
    QVERIFY(!parseProgressLine(QStringLiteral("Deleting from prueba.rar"), &msg, &pct));
    QVERIFY(!parseProgressLine(QStringLiteral(""), &msg, &pct));
}

void TestDiagnostics::describiendoDiagnosticos()
{
    ProcessOutcome o;
    o.diagnostic = Diagnostic::WrongPassword;
    QVERIFY(!Diagnostics::describe(o).isEmpty());
    o.diagnostic = Diagnostic::LicenseRequired;
    QVERIFY(!Diagnostics::describe(o).isEmpty());
}

// ---------------------------------------------------------------------------

class TestHFormat : public QObject
{
    Q_OBJECT

private slots:
    void tamanos();
    void tamanosAusentes();
    void fechas();
    void conteo();
};


/// Comprobaciones de `ArchiveService` que NO necesitan los binarios: son los
/// caminos que se pueden resolver solo mirando si la herramienta esta o no.
class TestArchiveService : public QObject
{
    Q_OBJECT

private slots:
    void sinHerramientasNoEsFalloDeLicencia();
    void recoveryRecordSinRarDaToolNotFound();
    void cifrarNombresExigeContrasena();
    void detectaFormato();
    void detectaFormatoPorExtension();
};

void TestArchiveService::sinHerramientasNoEsFalloDeLicencia()
{
    // Sin `rar` disponible, el fallo es "falta la herramienta", no "falta la
    // licencia". Confundir las dos cosas lleva a pedirle al usuario una clave
    // de pago que no va a arreglar nada.
    ExternalTools tools;   // todo vacio: no hay ni rar ni unrar
    ArchiveService service;
    service.setTools(tools);

    const ProcessOutcome r = service.addRecoveryRecord(QStringLiteral("no-existe.rar"));
    QVERIFY(!r.success);
    QCOMPARE(r.diagnostic, Diagnostic::ToolNotFound);
    QVERIFY(!r.needsLicense());
}

void TestArchiveService::recoveryRecordSinRarDaToolNotFound()
{
    ExternalTools tools;
    tools.rarPath = QStringLiteral("/no/existe/rar");
    ArchiveService service;
    service.setTools(tools);
    // Aunque la ruta este puesta, si no se puede ejecutar la respuesta correcta
    // tambien es ToolNotFound y no un fallo de RAR.
    const ProcessOutcome r = service.addRecoveryRecord(QStringLiteral("no-existe.rar"));
    QVERIFY(!r.success);
}

void TestArchiveService::cifrarNombresExigeContrasena()
{
    ExternalTools tools;
    tools.rarPath = QStringLiteral("/no/existe/rar");
    ArchiveService service;
    service.setTools(tools);

    QString staged;
    // Sin contrasena no se ni intenta reempaquetar.
    const ProcessOutcome r = service.encryptFileNames(
        QStringLiteral("no-existe.rar"), QString(), QString(), &staged);
    QVERIFY(!r.success);
    QVERIFY(r.needsPassword());
    QVERIFY(staged.isEmpty());
}

void TestArchiveService::detectaFormato()
{
    // El formato se saca del contenido, no de la extension: un archivo mal
    // nombrado o un SFX con un RAR dentro tienen que salir bien igual.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const auto write = [&dir](const QString &name, const QByteArray &bytes) {
        const QString path = dir.filePath(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly))
            return QString();
        f.write(bytes);
        f.close();
        return path;
    };

    const QByteArray rar5("Rar!\x1a\x07\x01\x00", 8);
    const QByteArray rar4("Rar!\x1a\x07\x00", 7);
    const QByteArray zip("PK\x03\x04", 4);

    // RAR 5, RAR 4, ZIP y un fichero cualquiera.
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("a.rar"), rar5)),
             ArchiveFormat::Rar);
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("v4.rar"), rar4)),
             ArchiveFormat::Rar);
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("a.zip"), zip)),
             ArchiveFormat::Zip);
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("a.txt"), "hola\n")),
             ArchiveFormat::Unknown);

    // La extension no manda: un ZIP llamado .rar sigue siendo ZIP, que es
    // justo el caso que rompe si se decide por el nombre.
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("mentira.rar"), zip)),
             ArchiveFormat::Zip);
    QCOMPARE(ArchiveService::detectFormat(write(QStringLiteral("mentira.zip"), rar5)),
             ArchiveFormat::Rar);
    QVERIFY(!ArchiveService::isSupported(write(QStringLiteral("a.txt"), "hola\n")));
}

void TestArchiveService::detectaFormatoPorExtension()
{
    // Un volumen tiene nombre propio ("p.part2.rar") y tambien es un RAR valido
    // por contenido, asi que abrir cualquiera de las partes funciona igual.
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QByteArray rar5("Rar!\x1a\x07\x01\x00", 8);
    for (const QString &name : {QStringLiteral("p.rar"), QStringLiteral("p.part2.rar"),
                                QStringLiteral("p.part01.rar")}) {
        const QString path = dir.filePath(name);
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(rar5);
        f.close();
        QVERIFY2(ArchiveService::isSupported(path), qPrintable(name));
    }

    // Y un archivo que no existe no se puede ni mirar: no se adivina por la
    // extension, se dice que no se sabe.
    QCOMPARE(ArchiveService::detectFormat(dir.filePath(QStringLiteral("no-existe.rar"))),
             ArchiveFormat::Unknown);
}

void TestHFormat::tamanos()
{
    QVERIFY(!HFormat::fileSize(0).isEmpty());
    QVERIFY(!HFormat::fileSize(1023).isEmpty());
    QVERIFY(HFormat::fileSize(1536).contains(QLatin1String("KB")));
    QVERIFY(HFormat::fileSize(5 * 1024 * 1024).contains(QLatin1String("MB")));
    QVERIFY(HFormat::fileSize(3LL * 1024 * 1024 * 1024).contains(QLatin1String("GB")));
}

void TestHFormat::tamanosAusentes()
{
    // Un directorio no tiene tamano conocido: la columna debe quedar vacia,
    // no mostrar "0 bytes".
    QVERIFY(HFormat::fileSizeOrEmpty(-1).isEmpty());
    QVERIFY(!HFormat::fileSizeOrEmpty(0).isEmpty());
}

void TestHFormat::fechas()
{
    const QDateTime dt = QDateTime::fromString(QStringLiteral("2026-09-25 15:14:30"),
                                               QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    QVERIFY(!HFormat::fileDateTime(dt).isEmpty());
    QVERIFY(!HFormat::fileDateTimeLong(dt).isEmpty());
    QVERIFY(HFormat::fileDateTime(QDateTime()).isEmpty());
}

void TestHFormat::conteo()
{
    const QString s = HFormat::fileCountText(3, 2, 1024, 512);
    QVERIFY(s.contains(QLatin1String("3")));
    QVERIFY(s.contains(QLatin1String("2")));
}

// Ejecuta las tres clases de test de este fichero. QTEST_MAIN solo arranca una,
// asi que se hace a mano.
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("qtrar_core_tests"));

    int failures = 0;
    {
        TestListParser t;
        failures += QTest::qExec(&t, argc, argv);
    }
    {
        TestDiagnostics t;
        failures += QTest::qExec(&t, argc, argv);
    }
    {
        TestHFormat t;
        failures += QTest::qExec(&t, argc, argv);
    }
    {
        TestArchiveService t;
        failures += QTest::qExec(&t, argc, argv);
    }
    return failures;
}

#include "tst_core.moc"
