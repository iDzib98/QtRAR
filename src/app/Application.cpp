#include "Application.h"

#include "ui/MainWindow.h"
#include "ui/ThemeManager.h"

#include <QDir>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QSettings>
#include <QStyleFactory>

namespace qtrar {

namespace {
const char *kSocketName = "QtRAR-single-instance";
} // namespace

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    setApplicationName(QStringLiteral("QtRAR"));
    setApplicationDisplayName(QStringLiteral("QtRAR"));
    setApplicationVersion(QStringLiteral(QTRAR_VERSION));
    setOrganizationName(QStringLiteral("QtRAR"));
    setOrganizationDomain(QStringLiteral("qtrar.local"));
    setWindowIcon(ThemeManager::icon(QStringLiteral("app")));

    // Fusion de estilos: "Fusion" es el que mas se parece al aspecto clasico
    // de WinRAR en Linux.
    if (QStyle *style = QStyleFactory::create(QStringLiteral("Fusion")))
        setStyle(style);
}

Application::~Application() = default;

void Application::loadTranslations()
{
    QSettings settings;
    m_language = settings.value(QStringLiteral("ui/language")).toString();
    if (m_language.isEmpty())
        m_language = QLocale::system().name();   // p.ej. "es_ES"

    // Puede que no haya traduccion de Qt para ese idioma: es normal, no es un
    // error, y por eso el resultado se ignora a proposito.
    const bool qtLoaded =
        m_qtTranslator.load(QStringLiteral("qtbase") + QLatin1Char('_') + m_language,
                            QLibraryInfo::path(QLibraryInfo::TranslationsPath))
        || m_qtTranslator.load(QStringLiteral("qtbase") + QLatin1Char('_') + m_language.left(2),
                               QLibraryInfo::path(QLibraryInfo::TranslationsPath));
    if (qtLoaded)
        installTranslator(&m_qtTranslator);

    // Se buscan las traducciones junto al ejecutable (instalacion) y en el
    // arbol de fuentes (desarrollo).
    const QStringList searchPaths = {
        QCoreApplication::applicationDirPath() + QStringLiteral("/i18n"),
        QCoreApplication::applicationDirPath() + QStringLiteral("/../share/qtrar/i18n"),
    };
    for (const QString &base : searchPaths) {
        if (m_appTranslator.load(QStringLiteral("qtrar_") + m_language, base)
            || m_appTranslator.load(QStringLiteral("qtrar_") + m_language.left(2), base)) {
            installTranslator(&m_appTranslator);
            return;
        }
    }
}

bool Application::setup(const QStringList &arguments)
{
    m_parser.setApplicationDescription(
        tr("Gestor de archivos con la interfaz de WinRAR para Linux.\n"
           "Se apoya en los binarios oficiales de RAR (unrar/rar) y en 7-Zip para ZIP."));
    const QCommandLineOption helpOpt = m_parser.addHelpOption();
    const QCommandLineOption versionOpt = m_parser.addVersionOption();
    m_parser.addPositionalArgument(QStringLiteral("archivo"),
                                   tr("Archivo(s) RAR o ZIP; acción contextual según las opciones."),
                                   QStringLiteral("[archivos...]"));

    const QCommandLineOption langOpt({QStringLiteral("l"), QStringLiteral("lang")},
                                     tr("Idioma de la interfaz (es, en)."),
                                     QStringLiteral("code"));
    m_parser.addOption(langOpt);

    const QCommandLineOption themeOpt(QStringLiteral("theme"),
                                      tr("Tema de la interfaz: original o system."),
                                      QStringLiteral("name"));
    m_parser.addOption(themeOpt);

    const QCommandLineOption extractOpt(QStringLiteral("extract-to"),
                                        tr("Extrae el archivo en la carpeta indicada y termina."),
                                        QStringLiteral("dir"));
    m_parser.addOption(extractOpt);

    // Sonda de pruebas: vuelca el estado del archivo y termina.
    const QCommandLineOption dumpOpt(QStringLiteral("dump"),
                                     tr("Volca el listado y termina (pruebas)."));
    m_parser.addOption(dumpOpt);

    const QCommandLineOption testOpt(QStringLiteral("test"),
                                     tr("Comprueba la integridad del archivo y termina."));
    m_parser.addOption(testOpt);
    const QCommandLineOption extractHereOpt(QStringLiteral("extract-here"),
                                            tr("Extrae el archivo junto a su ubicación y termina."));
    m_parser.addOption(extractHereOpt);
    const QCommandLineOption extractDialogOpt(QStringLiteral("extract-to-dialog"),
                                              tr("Abre el diálogo para extraer el archivo."));
    m_parser.addOption(extractDialogOpt);
    const QCommandLineOption addArchiveOpt(QStringLiteral("add-to-archive"),
                                           tr("Abre el diálogo para añadir los archivos seleccionados."));
    m_parser.addOption(addArchiveOpt);

    // `addHelpOption()` y `addVersionOption()` ya atienden -h/-v y terminan
    // la ejecucion; solo hay que propagar un error de sintaxis.
    if (!m_parser.parse(arguments)) {
        m_parser.showHelp(1);
        return false;
    }

    // `parse()` no ejecuta las opciones de salida; eso solo lo hace
    // `process()`. Como tambien tenemos que guardar preferencias despues de
    // analizar, atendemos help/version aqui para que no arranquen una ventana.
    if (m_parser.isSet(helpOpt)) {
        m_parser.showHelp(0);
        return false;
    }
    if (m_parser.isSet(versionOpt)) {
        m_parser.showVersion();
        return false;
    }


    if (m_parser.isSet(langOpt))
        QSettings().setValue(QStringLiteral("ui/language"), m_parser.value(langOpt));
    if (m_parser.isSet(themeOpt)) {
        const QString t = m_parser.value(themeOpt);
        QSettings().setValue(QStringLiteral("ui/theme"), t == QLatin1String("system") ? 1 : 0);
    }

    ThemeManager::apply();
    // Sigue los cambios de claro/oscuro del escritorio mientras se usa.
    ThemeManager::watchSystemColorScheme();
    loadTranslations();
    m_dumpListing = m_parser.isSet(dumpOpt);
    m_test = m_parser.isSet(testOpt);
    m_extractToDialog = m_parser.isSet(extractDialogOpt);
    m_addToArchive = m_parser.isSet(addArchiveOpt);
    const bool extractHere = m_parser.isSet(extractHereOpt);

    const QStringList actionArgs = m_parser.positionalArguments();
    const int actionCount = int(m_test) + int(extractHere) + int(m_extractToDialog)
        + int(m_addToArchive) + int(m_parser.isSet(extractOpt));
    if (actionCount > 1) {
        QTextStream(stderr) << QCoreApplication::translate(
            "qtrar::Application", "qtrar: selecciona una sola acción de archivo.") << Qt::endl;
        return false;
    }
    if ((m_test || extractHere || m_extractToDialog) && actionArgs.size() != 1) {
        QTextStream(stderr) << QCoreApplication::translate(
            "qtrar::Application", "qtrar: esta acción requiere exactamente un archivo.") << Qt::endl;
        return false;
    }
    if (m_addToArchive && actionArgs.isEmpty()) {
        QTextStream(stderr) << QCoreApplication::translate(
            "qtrar::Application", "qtrar: --add-to-archive requiere archivos seleccionados.")
                            << Qt::endl;
        return false;
    }
    if (extractHere)
        m_extractTo = QFileInfo(actionArgs.first()).absolutePath();

    // `--extract-to` es una operacion de verdad, no una opcion decorativa: si
    // se pide sin archivo no hay nada que extraer y se dice, en vez de abrir
    // una ventana vacia.
    if (m_parser.isSet(extractOpt)) {
        m_extractTo = QDir(m_parser.value(extractOpt)).absolutePath();
        if (m_parser.positionalArguments().isEmpty()) {
            QTextStream(stderr) << QCoreApplication::translate(
                "qtrar::Application", "qtrar: --extract-to necesita un archivo que extraer.")
                                 << Qt::endl;
            return false;
        }
    }
    return true;
}

bool Application::claimPrimaryInstance()
{
    // Si el usuario lo ha desactivado, no se usa instancia unica.
    if (!QSettings().value(QStringLiteral("app/singleInstance"), true).toBool())
        return true;

    QLocalSocket probe;
    probe.connectToServer(QString::fromLatin1(kSocketName));
    if (probe.waitForConnected(300)) {
        // Ya hay una instancia: se le pasa el fichero y se sale.
        const QStringList args = m_parser.positionalArguments();
        probe.write(args.isEmpty() ? QByteArray() : args.first().toUtf8());
        probe.flush();
        probe.waitForBytesWritten(500);
        return false;
    }

    QLocalServer::removeServer(QString::fromLatin1(kSocketName));
    auto *server = new QLocalServer(this);
    if (!server->listen(QString::fromLatin1(kSocketName))) {
        // Si no se puede escuchar (permisos), se sigue como instancia unica
        // sin possibilidade de recibir peticiones: mejor que negarse a arrancar.
        return true;
    }
    connect(server, &QLocalServer::newConnection, this, [this, server] {
        QLocalSocket *socket = server->nextPendingConnection();
        if (!socket)
            return;
        connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);
        connect(socket, &QLocalSocket::readyRead, this, [this, socket] {
            const QString path = QString::fromUtf8(socket->readAll()).trimmed();
            if (!path.isEmpty())
                emit openArchiveRequested(path);
        });
    });
    return true;
}

} // namespace qtrar
