// Punto de entrada de QtRAR.
#include "app/Application.h"
#include "app/HeadlessProbe.h"
#include "ui/MainWindow.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QTimer>

namespace {

/// Monta la peticion sin interfaz. Las opciones vienen del parser de
/// `Application` y las sondas de pruebas del entorno, pero todas entran por el
/// mismo camino: aqui no se vuelve a interpretar `argv`.
qtrar::ProbeRequest probeRequest(const qtrar::Application &app, const QString &archive)
{
    qtrar::ProbeRequest r;
    r.archivePath = archive;
    r.dump = app.dumpListingRequested();
    r.extractTo = app.extractTo();
    r.password = qEnvironmentVariable("QTRAR_PASSWORD");

    r.volumes = qEnvironmentVariableIsSet("QTRAR_VOLUMES");
    r.test = app.testRequested() || qEnvironmentVariableIsSet("QTRAR_TEST");

    if (const QString to = qEnvironmentVariable("QTRAR_EXTRACT_TO"); !to.isEmpty())
        r.extractTo = to;
    if (const QString what = qEnvironmentVariable("QTRAR_EXTRACT_WHAT"); !what.isEmpty())
        r.extractItems = what.split(QLatin1Char(','), Qt::SkipEmptyParts);

    r.readComment = qEnvironmentVariableIsSet("QTRAR_READ_COMMENT");
    r.commentFile = qEnvironmentVariable("QTRAR_COMMENT_FILE");
    r.removeItems = qEnvironmentVariable("QTRAR_REMOVE");
    r.renameFrom = qEnvironmentVariable("QTRAR_RENAME_FROM");
    r.renameTo = qEnvironmentVariable("QTRAR_RENAME_TO");

    r.createPath = qEnvironmentVariable("QTRAR_CREATE");
    if (const QString files = qEnvironmentVariable("QTRAR_CREATE_FILES"); !files.isEmpty())
        r.createItems = files.split(QLatin1Char(':'), Qt::SkipEmptyParts);

    return r;
}

} // namespace

int main(int argc, char *argv[])
{
    qtrar::Application app(argc, argv);

    if (!app.setup(app.arguments()))
        return 1;

    // El parser de la linea de ordenes distingue opciones de ficheros: un
    // recorrido manual tomaria "en" (el valor de `-l`) por un archivo.
    const QStringList requested = app.archiveArguments();
    const QString archiveArg =
        requested.isEmpty() ? QString() : QDir(requested.first()).absolutePath();

    // Operaciones sin ventana: `--extract-to`, `--dump` y las sondas. Si hay
    // alguna se ejecuta y se sale, sin llegar a abrir la interfaz.
    int probeExit = 0;
    if (!app.hasGuiAction() && qtrar::runHeadlessProbe(probeRequest(app, archiveArg), probeExit))
        return probeExit;

    // Una acción del menú contextual es una petición independiente; pasarla a
    // la instancia existente como una mera ruta la degradaría a "abrir archivo".
    if (!app.hasGuiAction() && !app.claimPrimaryInstance())
        return 0;   // otra instancia ya se esta ocupando

    auto *window = new qtrar::MainWindow;
    QObject::connect(&app, &qtrar::Application::openArchiveRequested, window,
                     [window](const QString &path) { window->openSecondaryInstanceRequest(path); });
    window->show();

    // Abrir el archivo se encola para que el bucle de eventos ya este en
    // marcha: `openArchive()` puede mostrar un dialogo modal (contrasena,
    // formato no soportado) y sin bucle se quedaria colgado para siempre.
    if (app.addToArchiveRequested()) {
        const QStringList files = app.archiveArguments();
        QString suggested;
        if (files.size() == 1) {
            const QFileInfo fi(files.first());
            // No proponer el propio archivo seleccionado como destino cuando
            // se invoca la acción sobre un RAR/ZIP; la lista de entrada puede
            // incluir archivos comprimidos y QtRAR no debe sobrescribirlos.
            if (qtrar::ArchiveService::detectFormat(files.first()) == qtrar::ArchiveFormat::Unknown) {
                suggested = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
                    + QStringLiteral(".rar");
            }
        }
        QTimer::singleShot(0, window, [window, suggested, files] {
            window->createArchive(suggested, files);
        });
    } else if (app.extractToDialogRequested()) {
        QTimer::singleShot(0, window, [window, archiveArg] {
            if (window->openArchive(archiveArg, true))
                window->dispatchCommand(qtrar::CommandRegistry::CmdExtractTo);
        });
    } else if (!archiveArg.isEmpty()) {
        QTimer::singleShot(0, window, [window, archiveArg] { window->openArchive(archiveArg, true); });
    }

    // Sonda de interfaz: entrar en una carpeta interna antes de volcar.
    if (!qEnvironmentVariableIsEmpty("QTRAR_GOTO")) {
        QTimer::singleShot(1200, window,
                           [window] { window->goTo(qEnvironmentVariable("QTRAR_GOTO")); });
    }

    // Sonda de interfaz: volcar la ventana a PNG y a texto, y terminar.
    if (qEnvironmentVariableIsSet("QTRAR_SNAPSHOT")) {
        QTimer::singleShot(2500, window, [window] {
            if (!qEnvironmentVariableIsEmpty("QTRAR_SNAPSHOT"))
                window->grab().save(qEnvironmentVariable("QTRAR_SNAPSHOT"));
            QTextStream out(stdout);
            window->dumpUi(out);
            out.flush();
            QCoreApplication::exit(0);
        });
    }

    return app.exec();
}
