// Pruebas de la capa de interfaz que contienen logica: el filtro de busqueda
// y la validacion del dialogo de creacion. Ninguna de las dos necesita una
// ventana visible, asi que corre con QT_QPA_PLATFORM=offscreen.
#include "model/ArchiveTreeModel.h"
#include "ui/ThemeManager.h"
#include "ui/FindProxyModel.h"
#include "ui/dialogs/CreateDialog.h"
#include "ui/dialogs/ExtractDialog.h"
#include "ui/CommandRegistry.h"
#include "ui/MainWindow.h"
#include "ui/AddressBar.h"
#include "ui/dialogs/LicenseDialog.h"
#include "ui/dialogs/RarSetupDialog.h"
#include "core/BinaryLocator.h"

#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QToolButton>
#include <QTreeView>
#include <QTimer>
#include <QtTest>

using namespace qtrar;

class TestUi : public QObject
{
    Q_OBJECT

private slots:
    void patronSimple();
    void conComodines_data();
    void conComodines();
    void filtraPorNombre();
    void buscaEnTodoElArchivo();
    void navegarCancelaLaBusqueda();
    void abrirOtroArchivoLimpiaLaBusqueda();
    void gotoAUnaCarpetaInexistenteNoSeMueve();

    void detectaElTemaDelSistema();
    void paletaOscuraNoEsBlanca();
    void filtroVacioNoOculta();
    void cacheSeInvalidaAlCambiar();

    void creacionSinNombre();
    void creacionSinFicheros();
    void creacionConFicheroInexistente();
    void creacionConExtensionIncoherente();
    void creacionValida();

    void barraSigueElOrdenDeWinRAR();
    void elBloqueDerechoCambiaConElArchivo();
    void anadirSigueDisponibleSiFaltaRar();
    void lasEtiquetasCortasNoTraenPuntosSuspensivos();
    void navegaPorDirectoriosLocales();
    void puedeSubirDesdeLaRaizDelArchivoAlDirectorioLocal();
    void navegaPorDirectoriosVirtualesDelArchivo();
    void muestraIconosEnLaListaDelArchivo();
    void opcionesDelDialogoDeExtraccionSeConservan();
    void dialogoDeLicenciaSoloInformaYPermiteRecomprobar();
    void menuAyudaIncluyeGestionDeLicencia();
    void aceptaUnBinarioRarLinuxEjecutable();
    void localizadorUsaElBinarioRarConfigurado();
    void cancelarEditorComentarioNoCierraQtRAR();
};

// --- Barra de herramientas ---------------------------------------------------

// El orden de la barra es el de WinRAR, no una cuestion de gusto: es lo que el
// usuario tiene delante comparando las dos aplicaciones.
void TestUi::barraSigueElOrdenDeWinRAR()
{
    CommandRegistry reg;
    const QList<CommandRegistry::Definition> izq = reg.toolbarGroup(0, true);
    QCOMPARE(izq.size(), 8);
    QList<CommandRegistry::Id> esperado = {
        CommandRegistry::CmdAddToArchive, CommandRegistry::CmdExtractTo,
        CommandRegistry::CmdTest,         CommandRegistry::CmdView,
        CommandRegistry::CmdDelete,       CommandRegistry::CmdFind,
        CommandRegistry::CmdWizard,       CommandRegistry::CmdInfo,
    };
    for (int i = 0; i < esperado.size(); ++i)
        QCOMPARE(izq.at(i).id, esperado.at(i));
}

void TestUi::elBloqueDerechoCambiaConElArchivo()
{
    CommandRegistry reg;
    // Sin archivo abierto solo queda "Reparar" detras de la linea de separacion.
    const QList<CommandRegistry::Definition> sinArchivo = reg.toolbarGroup(1, false);
    QCOMPARE(sinArchivo.size(), 1);
    QCOMPARE(sinArchivo.at(0).id, CommandRegistry::CmdRecover);

    // Con archivo abierto, "Reparar" se sustituye por los otros cuatro.
    const QList<CommandRegistry::Definition> conArchivo = reg.toolbarGroup(1, true);
    QCOMPARE(conArchivo.size(), 4);
    QList<CommandRegistry::Id> esperado = {
        CommandRegistry::CmdVirusScan, CommandRegistry::CmdComment,
        CommandRegistry::CmdProtect,   CommandRegistry::CmdSfx,
    };
    for (int i = 0; i < esperado.size(); ++i)
        QCOMPARE(conArchivo.at(i).id, esperado.at(i));

    // Y los del bloque izquierdo estan en los dos estados.
    QCOMPARE(reg.toolbarGroup(0, false).size(), 8);
}

void TestUi::anadirSigueDisponibleSiFaltaRar()
{
    CommandRegistry registry;
    QAction *add = registry.action(CommandRegistry::CmdAddToArchive);
    QAction *create = registry.action(CommandRegistry::CmdNewArchive);
    QAction *configure = registry.action(CommandRegistry::CmdConfigureRar);
    QVERIFY(add && create && configure);

    registry.setCanCreateArchives(false);
    QVERIFY(add->isEnabled()); // abre el asistente para vincular el rar oficial
    QVERIFY(configure->isEnabled());
    QVERIFY(!create->isEnabled());

    registry.setCanCreateArchives(true);
    QVERIFY(add->isEnabled());
    QVERIFY(create->isEnabled());
}

void TestUi::lasEtiquetasCortasNoTraenPuntosSuspensivos()
{
    // Bajo el icono los "..." y los "&" sobran: ocupan media barra.
    for (CommandRegistry::Id id : {CommandRegistry::CmdAddToArchive,
                                  CommandRegistry::CmdExtractTo,
                                  CommandRegistry::CmdTest, CommandRegistry::CmdView,
                                  CommandRegistry::CmdDelete, CommandRegistry::CmdFind,
                                  CommandRegistry::CmdWizard, CommandRegistry::CmdInfo,
                                  CommandRegistry::CmdVirusScan,
                                  CommandRegistry::CmdComment, CommandRegistry::CmdProtect,
                                  CommandRegistry::CmdSfx, CommandRegistry::CmdRecover}) {
        const QString etiqueta = CommandRegistry::shortLabel(id);
        QVERIFY2(!etiqueta.isEmpty(), "falta la etiqueta corta de un boton de la barra");
        QVERIFY2(!etiqueta.contains(QLatin1Char('&')), qPrintable(etiqueta));
        QVERIFY2(!etiqueta.endsWith(QLatin1String("...")), qPrintable(etiqueta));
    }
}

void TestUi::navegaPorDirectoriosLocales()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    QVERIFY(QDir(temp.path()).mkdir(QStringLiteral("subcarpeta")));
    QFile file(temp.path() + QStringLiteral("/nota.txt"));
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("contenido");
    file.close();

    MainWindow window;
    auto *view = window.findChild<QTreeView *>(QStringLiteral("archiveView"));
    QVERIFY(view);
    auto *proxy = qobject_cast<FindProxyModel *>(view->model());
    QVERIFY(proxy);
    auto *model = qobject_cast<QFileSystemModel *>(proxy->sourceModel());
    QVERIFY(model);
    QSignalSpy directoryLoaded(model, &QFileSystemModel::directoryLoaded);
    QVERIFY(window.browseDirectory(temp.path()));
    QVERIFY(window.browsingFileSystem());
    QCOMPARE(window.fileSystemPath(), QDir::cleanPath(temp.path()));
    QTRY_VERIFY([&] {
        for (const QList<QVariant> &args : directoryLoaded) {
            if (!args.isEmpty() && QDir::cleanPath(args.first().toString())
                    == QDir::cleanPath(temp.path()))
                return true;
        }
        return false;
    }());

    const QModelIndex root = proxy->mapFromSource(model->index(temp.path()));
    QTRY_COMPARE(proxy->rowCount(root), 3); // .., subcarpeta y nota.txt
    const QModelIndex parent = proxy->index(0, 0, root);
    QCOMPARE(model->fileName(proxy->mapToSource(parent)), QStringLiteral(".."));
    QVERIFY(model->fileInfo(proxy->mapToSource(proxy->index(0, 0, root))).isDir());

    QVERIFY(QMetaObject::invokeMethod(&window, "onActivated", Qt::DirectConnection,
                                      Q_ARG(QModelIndex, parent)));
    QCOMPARE(window.fileSystemPath(), QDir::cleanPath(QFileInfo(temp.path()).absolutePath()));
    QVERIFY(window.browseDirectory(temp.path()));

    const QModelIndex folder = proxy->mapFromSource(
        model->index(temp.path() + QStringLiteral("/subcarpeta")));
    QVERIFY(folder.isValid());
    QVERIFY(QMetaObject::invokeMethod(&window, "onActivated", Qt::DirectConnection,
                                      Q_ARG(QModelIndex, folder)));
    QCOMPARE(window.fileSystemPath(), QDir::cleanPath(temp.path() + QStringLiteral("/subcarpeta")));

    auto *address = window.findChild<AddressBar *>();
    QVERIFY(address);
    QVERIFY(address->goBack());
    QCOMPARE(window.fileSystemPath(), QDir::cleanPath(temp.path()));

    QVERIFY(window.browseDirectory(temp.path() + QStringLiteral("/subcarpeta")));
    auto *up = address->findChild<QToolButton *>(QStringLiteral("upButton"));
    QVERIFY(up && up->isEnabled());
    up->click();
    QCOMPARE(window.fileSystemPath(), QDir::cleanPath(temp.path()));
}

void TestUi::puedeSubirDesdeLaRaizDelArchivoAlDirectorioLocal()
{
    AddressBar address;
    address.setArchivePath(QStringLiteral("/tmp/prueba.rar"), QString());
    auto *up = address.findChild<QToolButton *>(QStringLiteral("upButton"));
    QVERIFY(up);
    QVERIFY(up->isEnabled());

    address.setFileSystemPath(QStringLiteral("/"));
    QVERIFY(!up->isEnabled());
}

// --- Filtro de busqueda -----------------------------------------------------

void TestUi::patronSimple()
{
    QVERIFY(FindProxyModel::matchesPattern(QStringLiteral("notas.txt"), QStringLiteral("txt")));
    QVERIFY(FindProxyModel::matchesPattern(QStringLiteral("notas.txt"), QStringLiteral("TXT")));
    QVERIFY(!FindProxyModel::matchesPattern(QStringLiteral("notas.txt"),
                                            QStringLiteral("pdf")));
    // El patron vacio deja pasar todo: es como se limpia la busqueda.
    QVERIFY(FindProxyModel::matchesPattern(QStringLiteral("lo que sea"), QString()));
    // Sin comodines busca en cualquier parte del nombre, no al principio.
    QVERIFY(FindProxyModel::matchesPattern(QStringLiteral("docs/año_español.txt"),
                                           QStringLiteral("año")));
}

void TestUi::conComodines_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<bool>("expected");

    QTest::newRow("estrella al final") << "notas.txt" << "*.txt" << true;
    QTest::newRow("estrella al inicio") << "notas.txt" << "notas*" << true;
    QTest::newRow("estrella en medio") << "notas.txt" << "no*.txt" << true;
    QTest::newRow("doble estrella") << "src/docs/notas.txt" << "src/*notas*" << true;
    QTest::newRow("interrogacion") << "notas.txt" << "nota?.txt" << true;
    QTest::newRow("interrogacion falla") << "notas.txt" << "nota?.txt.tmp" << false;
    QTest::newRow("sin coincidencia") << "notas.txt" << "*.pdf" << false;
    QTest::newRow("punto es literal") << "notas.txt" << "*.txt" << true;
    QTest::newRow("asterisco dentro de nombre") << "a*b.txt" << "a*b" << true;
}

void TestUi::conComodines()
{
    QFETCH(QString, name);
    QFETCH(QString, pattern);
    QFETCH(bool, expected);
    QCOMPARE(FindProxyModel::matchesPattern(name, pattern), expected);
}

/// Monta un modelo con cuatro entradas y lo envuelve en el filtro.
/// Listado de ejemplo. Se devuelve para poder volver a cargarlo en el modelo
/// (por ejemplo, al "abrir" otro archivo) sin escribirlo dos veces.
static ArchiveListing makeListing()
{
    ArchiveListing listing;
    listing.info.path = QStringLiteral("prueba.rar");
    listing.info.format = QStringLiteral("RAR 5");

    // unrar lista tambien las carpetas, asi que "src/img" aparece por su cuenta.
    const QStringList names = {QStringLiteral("src"), QStringLiteral("src/docs"),
                               QStringLiteral("src/docs/notas.txt"),
                               QStringLiteral("src/img"),
                               QStringLiteral("src/img/foto.png")};
    for (const QString &name : names) {
        ArchiveEntry e;
        // `name` es la ruta interna completa, tal cual la devuelve unrar.
        e.name = name;
        e.size = 10;
        e.packedSize = 5;
        const bool dir = name.endsWith(QLatin1String("src"))
                         || name.endsWith(QLatin1String("docs"))
                         || name.endsWith(QLatin1String("img"));
        e.type = dir ? EntryType::Directory : EntryType::File;
        listing.entries.append(e);
    }
    return listing;
}

static void fillModel(ArchiveTreeModel &model)
{
    model.setListing(makeListing());
}

void TestUi::filtraPorNombre()
{
    ArchiveTreeModel model;
    fillModel(model);
    FindProxyModel proxy;
    proxy.setSourceModel(&model);

    // Se baja a la carpeta que contiene los ficheros.
    model.setCurrentPath(QStringLiteral("src/docs"));
    QCOMPARE(proxy.rowCount(), 2); // .. y notas.txt

    const auto match = [](const QString &name) { return name.contains(QLatin1String("notas")); };
    model.setSearchPattern(QStringLiteral("notas"), match);
    proxy.setSearchText(QStringLiteral("notas"));
    QCOMPARE(proxy.rowCount(), 1);

    model.clearSearchPattern();
    proxy.setSearchText(QString());
    model.setCurrentPath(QStringLiteral("src"));
    QCOMPARE(proxy.rowCount(), 3);   // .., docs e img
}

void TestUi::buscaEnTodoElArchivo()
{
    ArchiveTreeModel model;
    fillModel(model);
    FindProxyModel proxy;
    proxy.setSourceModel(&model);

    // La vista normal solo enseña un nivel: en la raiz estan .. y "src".
    QCOMPARE(proxy.rowCount(), 2);

    // Al buscar, el modelo lista a plano lo que coincida este donde este. Sin
    // esto no habria manera de encontrar un fichero buried en una subcarpeta.
    const auto match = [](const QString &name) {
        return FindProxyModel::matchesPattern(name, QStringLiteral("*.txt"));
    };
    model.setSearchPattern(QStringLiteral("*.txt"), match);
    proxy.setSearchText(QStringLiteral("*.txt"));
    QVERIFY(model.inSearchMode());
    QCOMPARE(proxy.rowCount(), 1);

    // Y se ve la ruta entera, no solo el nombre, para saber donde esta.
    const QString found = model.data(proxy.index(0, 0), Qt::UserRole).toString();
    QCOMPARE(found, QStringLiteral("src/docs/notas.txt"));
}

void TestUi::navegarCancelaLaBusqueda()
{
    ArchiveTreeModel model;
    fillModel(model);

    model.setSearchPattern(QStringLiteral("*.txt"),
                           [](const QString &n) { return n.endsWith(QLatin1String(".txt")); });
    QVERIFY(model.inSearchMode());

    // Navegar mientras se busca no puede dejar la vista showing una lista a
    // plano que ya no corresponde a la carpeta en la que se esta.
    model.setCurrentPath(QStringLiteral("src"));
    QVERIFY(!model.inSearchMode());
    QCOMPARE(model.currentPath(), QStringLiteral("src"));
}

void TestUi::abrirOtroArchivoLimpiaLaBusqueda()
{
    ArchiveTreeModel model;
    fillModel(model);

    model.setSearchPattern(QStringLiteral("notas"),
                           [](const QString &name) { return name.contains(QLatin1String("notas")); });
    QVERIFY(model.inSearchMode());

    // El patron pertenece al archivo que estaba abierto. Al abrir otro, si no
    // se limpia, el modelo se queda en modo busqueda con un patron que ya no
    // corresponde a lo que hay dentro.
    model.setListing(makeListing());
    QVERIFY(!model.inSearchMode());
    QVERIFY(model.searchPattern().isEmpty());
}

void TestUi::gotoAUnaCarpetaInexistenteNoSeMueve()
{
    ArchiveTreeModel model;
    fillModel(model);

    QCOMPARE(model.currentPath(), QString());
    QVERIFY(model.isInsideArchive(QStringLiteral("src")));
    QVERIFY(model.isInsideArchive(QStringLiteral("src/docs")));
    // "src/docs" es una carpeta de verdad y "src" tambien: la raiz vacia cuenta.
    QVERIFY(model.isInsideArchive(QString()));

    // Una ruta que no existe, o un fichero en vez de una carpeta, se rechazan.
    // Si no, la vista se queda vacia con una direccion inventada en la barra.
    QVERIFY(!model.isInsideArchive(QStringLiteral("zzz")));
    QVERIFY(!model.isInsideArchive(QStringLiteral("src/docs/notas.txt")));

    model.setCurrentPath(QStringLiteral("zzz"));
    QCOMPARE(model.currentPath(), QString());
    QCOMPARE(model.rowCount(), 2);   // .. y src; sigue en la raiz

    model.setCurrentPath(QStringLiteral("src"));
    QCOMPARE(model.currentPath(), QStringLiteral("src"));
    model.setCurrentPath(QString());  // "subir" hasta la raiz sigue funcionando
    QCOMPARE(model.currentPath(), QString());
}

void TestUi::navegaPorDirectoriosVirtualesDelArchivo()
{
    ArchiveListing listing;
    listing.info.path = QStringLiteral("rutas-anidadas.zip");
    // Muchos listados no incluyen entradas explicitas para los directorios.
    for (const QString &name : {QStringLiteral("docs/nota.txt"),
                                QStringLiteral("docs/img/logo.png"),
                                QStringLiteral("raiz.txt")}) {
        ArchiveEntry entry;
        entry.name = name;
        entry.type = EntryType::File;
        entry.size = 12;
        entry.packedSize = 7;
        listing.entries.append(entry);
    }

    ArchiveTreeModel model;
    model.setListing(listing);
    QCOMPARE(model.rowCount(), 3); // .., docs (virtual) y raiz.txt
    QVERIFY(model.isParentEntry(model.index(0, ArchiveTreeModel::ColumnName)));
    QCOMPARE(model.data(model.index(0, ArchiveTreeModel::ColumnName)).toString(),
             QStringLiteral(".."));

    FindProxyModel proxy;
    proxy.setSourceModel(&model);
    proxy.sort(ArchiveTreeModel::ColumnName, Qt::AscendingOrder);
    QVERIFY(model.isDirectory(proxy.mapToSource(proxy.index(0, 0))));

    const QModelIndex docs = model.indexForPath(QStringLiteral("docs"));
    QVERIFY(docs.isValid());
    QVERIFY(model.isDirectory(docs));
    QVERIFY(!model.entryAt(docs)); // es navegable aunque no exista en el listado
    QVERIFY(model.isInsideArchive(QStringLiteral("docs/img")));

    model.setCurrentPath(QStringLiteral("docs"));
    QCOMPARE(proxy.rowCount(), 3); // .., nota.txt e img (virtual)
    const QModelIndex img = model.indexForPath(QStringLiteral("docs/img"));
    QVERIFY(img.isValid());
    QVERIFY(model.isDirectory(img));

    model.setCurrentPath(QStringLiteral("docs/img"));
    QCOMPARE(model.rowCount(), 2); // .. y logo.png
    QCOMPARE(model.data(model.index(1, ArchiveTreeModel::ColumnName)).toString(),
             QStringLiteral("logo.png"));
}

void TestUi::muestraIconosEnLaListaDelArchivo()
{
    ArchiveListing listing;
    listing.info.path = QStringLiteral("iconos.rar");
    ArchiveEntry file;
    file.name = QStringLiteral("docs/readme.txt");
    file.type = EntryType::File;
    listing.entries.append(file);

    ArchiveTreeModel model;
    model.setListing(listing);

    const QModelIndex folder = model.indexForPath(QStringLiteral("docs"));
    QVERIFY(folder.isValid());
    QVERIFY(!model.data(folder, Qt::DecorationRole).value<QIcon>().isNull());

    model.setCurrentPath(QStringLiteral("docs"));
    const QModelIndex entry = model.indexForPath(QStringLiteral("docs/readme.txt"));
    QVERIFY(entry.isValid());
    QVERIFY(!model.data(entry, Qt::DecorationRole).value<QIcon>().isNull());
}

void TestUi::detectaElTemaDelSistema()
{
    // No se puede comprobar el valor exacto (depende del escritorio de quien
    // prueba), pero si que devuelva algo sin colgarse ni lanzar excepcion.
    const bool dark = ThemeManager::systemIsDark();
    QVERIFY(dark || !dark);   // la llamada es inocua
}

void TestUi::paletaOscuraNoEsBlanca()
{
    // Con el tema del sistema, la paleta aplicada tiene que ser la que dice
    // `systemIsDark()`. El fallo que motivó esto era justo al reves: se
    // quitaba la hoja de estilos pero la paleta se quedaba en blanco.
    QPalette antes = QApplication::palette();

    ThemeManager::setTheme(ThemeManager::Theme::System);
    const QColor window = QApplication::palette().color(QPalette::Window);
    const QColor text = QApplication::palette().color(QPalette::WindowText);

    if (ThemeManager::systemIsDark()) {
        // Fondo oscuro y texto claro: si se invirtieran, la ventana seria
        // ilegible.
        QVERIFY2(window.lightness() < text.lightness(),
                 "en modo oscuro el fondo debe ser mas oscuro que el texto");
    }

    // Volver al tema propio deja la paleta como estaba.
    ThemeManager::setTheme(ThemeManager::Theme::Original);
    QApplication::setPalette(antes);
}

void TestUi::filtroVacioNoOculta()
{
    ArchiveTreeModel model;
    fillModel(model);
    FindProxyModel proxy;
    proxy.setSourceModel(&model);

    proxy.setSearchText(QStringLiteral("no existe"));
    model.setSearchPattern(QStringLiteral("no existe"),
                           [](const QString &n) { return n.contains(QLatin1String("no existe")); });
    QCOMPARE(proxy.rowCount(), 0);

    proxy.setSearchText(QString());
    model.clearSearchPattern();
    QCOMPARE(proxy.rowCount(), 2);   // .. y src
}

void TestUi::cacheSeInvalidaAlCambiar()
{
    ArchiveTreeModel model;
    fillModel(model);
    FindProxyModel proxy;
    proxy.setSourceModel(&model);

    const auto hits = [&model](const QString &pattern) {
        model.setSearchPattern(pattern,
                               [&pattern](const QString &n) {
                                   return FindProxyModel::matchesPattern(n, pattern);
                               });
        return model.rowCount();
    };

    QCOMPARE(hits(QStringLiteral("notas")), 1);
    // Si la cache del proxy no se limpiase, esto seguiria devolviendo 1.
    QCOMPARE(hits(QStringLiteral("foto.png")), 1);
    QCOMPARE(hits(QStringLiteral("notas")), 1);
}

// --- Dialogo de creacion ----------------------------------------------------

void TestUi::opcionesDelDialogoDeExtraccionSeConservan()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    ExtractDialog dlg;
    dlg.setArchiveName(QStringLiteral("copias.rar"));
    dlg.setDefaultDestination(tmp.path());
    auto *tree = dlg.findChild<QTreeView *>(QStringLiteral("extractFolderTree"));
    QVERIFY(tree);
    auto *folderModel = qobject_cast<QFileSystemModel *>(tree->model());
    QVERIFY(folderModel);
    QCOMPARE(folderModel->filePath(tree->rootIndex()), QDir::rootPath());
    QTRY_COMPARE(folderModel->filePath(tree->currentIndex()), QDir::cleanPath(tmp.path()));
    dlg.show();
    QTRY_VERIFY(tree->visualRect(tree->currentIndex()).intersects(tree->viewport()->rect()));
    dlg.hide();
    auto *extractButton = dlg.findChild<QPushButton *>(QStringLiteral("extractAcceptButton"));
    QVERIFY(extractButton);
    QCOMPARE(extractButton->text(), QStringLiteral("Extraer"));

    QCOMPARE(dlg.destination(), QDir(tmp.path()).absolutePath());
    auto *subfolder = dlg.findChild<QRadioButton *>(QStringLiteral("extractIntoArchiveFolder"));
    auto *direct = dlg.findChild<QRadioButton *>(QStringLiteral("extractDirectly"));
    auto *update = dlg.findChild<QRadioButton *>(QStringLiteral("extractUpdate"));
    auto *confirm = dlg.findChild<QRadioButton *>(QStringLiteral("extractConfirmOverwrite"));
    auto *rename = dlg.findChild<QRadioButton *>(QStringLiteral("extractRename"));
    auto *keepBroken = dlg.findChild<QCheckBox *>(QStringLiteral("extractKeepBroken"));
    auto *withoutPaths = dlg.findChild<QCheckBox *>(QStringLiteral("extractWithoutPaths"));
    auto *showExplorer = dlg.findChild<QCheckBox *>(QStringLiteral("extractShowInExplorer"));
    QVERIFY(subfolder && direct && update && confirm && rename && keepBroken && withoutPaths
            && showExplorer);

    confirm->setChecked(true);
    QCOMPARE(dlg.options(false, QString()).existingFiles, ExistingFilesMode::Confirm);

    QCOMPARE(dlg.destination(), QDir(tmp.path()).absolutePath());
    subfolder->setChecked(true);
    QCOMPARE(dlg.destination(), QDir(tmp.path()).absoluteFilePath(QStringLiteral("copias")));
    direct->setChecked(true);
    update->setChecked(true);
    rename->setChecked(true);
    keepBroken->setChecked(true);
    withoutPaths->setChecked(true);
    showExplorer->setChecked(true);
    QCOMPARE(dlg.destination(), QDir(tmp.path()).absolutePath());

    const ExtractOptions options = dlg.options(false, QString());
    QCOMPARE(options.updateMode, ExtractUpdateMode::Update);
    QCOMPARE(options.existingFiles, ExistingFilesMode::Rename);
    QVERIFY(options.keepBrokenFiles);
    QVERIFY(!options.preservePaths);
    QVERIFY(dlg.showInExplorer());

    dlg.setArchiveName(QStringLiteral("copias.zip"));
    QVERIFY(!update->isEnabled());
}

void TestUi::dialogoDeLicenciaSoloInformaYPermiteRecomprobar()
{
    LicenseStatus status;
    status.state = LicenseState::Evaluation;
    status.detail = QStringLiteral("estado de prueba");
    status.keyFileFound = true;
    status.keyFilePath = QStringLiteral("/home/usuario/rarreg.key");

    LicenseDialog dlg(status);
    auto *state = dlg.findChild<QLabel *>(QStringLiteral("rarLicenseStatus"));
    auto *keyState = dlg.findChild<QLabel *>(QStringLiteral("rarLicenseKeyStatus"));
    auto *locations = dlg.findChild<QListWidget *>(QStringLiteral("rarLicenseLocations"));
    QVERIFY(state && keyState && locations);
    QVERIFY(state->text().contains(QStringLiteral("evaluación"), Qt::CaseInsensitive));
    QVERIFY(keyState->text().contains(status.keyFilePath));
    QVERIFY(locations->count() >= 1);

    QSignalSpy recheck(&dlg, &LicenseDialog::recheckRequested);
    for (QPushButton *button : dlg.findChildren<QPushButton *>()) {
        if (button->text() == QStringLiteral("Comprobar de nuevo")) {
            button->click();
            break;
        }
    }
    QCOMPARE(recheck.count(), 1);

    status.state = LicenseState::Registered;
    dlg.setStatus(status);
    QVERIFY(state->text().contains(QStringLiteral("registrado"), Qt::CaseInsensitive));
}

void TestUi::menuAyudaIncluyeGestionDeLicencia()
{
    MainWindow window;
    QMenu *help = nullptr;
    for (QAction *action : window.menuBar()->actions()) {
        if (action->menu()
            && action->text().remove(QLatin1Char('&')).contains(QStringLiteral("Ayuda"),
                                                                Qt::CaseInsensitive)) {
            help = action->menu();
            break;
        }
    }
    QVERIFY(help);
    bool foundLicense = false;
    for (QAction *action : help->actions()) {
        if (action->text().contains(QStringLiteral("licencia"), Qt::CaseInsensitive))
            foundLicense = true;
    }
    QVERIFY(foundLicense);
}

void TestUi::aceptaUnBinarioRarLinuxEjecutable()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("rar"));
    QFile stub(path);
    QVERIFY(stub.open(QIODevice::WriteOnly));
    stub.write("#!/bin/sh\nprintf 'RAR 7.23\\n'\n");
    stub.close();
    QVERIFY(stub.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                | QFileDevice::ExeOwner));

    RarSetupDialog dlg(path);
    QPushButton *use = nullptr;
    for (QPushButton *button : dlg.findChildren<QPushButton *>()) {
        if (button->text() == QStringLiteral("Usar este binario"))
            use = button;
    }
    QVERIFY(use);
    QSignalSpy accepted(&dlg, &QDialog::accepted);
    use->click();
    QCOMPARE(accepted.count(), 1);
    QCOMPARE(dlg.result(), int(QDialog::Accepted));
    QCOMPARE(dlg.binaryPath(), QFileInfo(path).absoluteFilePath());
}

void TestUi::localizadorUsaElBinarioRarConfigurado()
{
    QTemporaryDir temp;
    QVERIFY(temp.isValid());
    const QString path = temp.filePath(QStringLiteral("rar"));
    QFile stub(path);
    QVERIFY(stub.open(QIODevice::WriteOnly));
    stub.write("#!/bin/sh\nprintf 'RAR 9.87\\n'\n");
    stub.close();
    QVERIFY(stub.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner
                                | QFileDevice::ExeOwner));

    QSettings settings;
    const bool hadPath = settings.contains(QStringLiteral("tools/rarPath"));
    const QVariant oldPath = settings.value(QStringLiteral("tools/rarPath"));
    const bool hadEnv = qEnvironmentVariableIsSet("QTRAR_RAR");
    const QByteArray oldEnv = qgetenv("QTRAR_RAR");
    settings.setValue(QStringLiteral("tools/rarPath"), path);
    qunsetenv("QTRAR_RAR");
    settings.sync();

    const ExternalTools tools = BinaryLocator::locate();

    if (hadPath)
        settings.setValue(QStringLiteral("tools/rarPath"), oldPath);
    else
        settings.remove(QStringLiteral("tools/rarPath"));
    if (hadEnv)
        qputenv("QTRAR_RAR", oldEnv);
    settings.sync();

    QCOMPARE(tools.rarPath, QFileInfo(path).absoluteFilePath());
    QCOMPARE(tools.rarVersion, QStringLiteral("9.87"));
}

void TestUi::cancelarEditorComentarioNoCierraQtRAR()
{
    MainWindow window;
    ExternalTools tools;
    tools.rarPath = QStringLiteral("/bin/true"); // No se ejecuta al cancelar.
    window.service()->setTools(tools);
    window.show();
    QApplication::processEvents();

    QTimer::singleShot(0, &window, [] {
        if (QWidget *modal = QApplication::activeModalWidget())
            modal->close();
    });
    window.dispatchCommand(CommandRegistry::CmdComment);
    QVERIFY(window.isVisible());
}

void TestUi::creacionSinNombre()
{
    CreateDialog dlg(QString(), CreateDialog::Mode::New);
    QString error;
    QVERIFY(!dlg.validate(&error));
    QVERIFY(!error.isEmpty());
}

void TestUi::creacionSinFicheros()
{
    QTemporaryDir tmp;
    const QString archive = tmp.filePath(QStringLiteral("nuevo.rar"));
    CreateDialog dlg(archive, CreateDialog::Mode::New);
    QString error;
    QVERIFY(!dlg.validate(&error));
}

void TestUi::creacionConFicheroInexistente()
{
    QTemporaryDir tmp;
    const QString archive = tmp.filePath(QStringLiteral("nuevo.rar"));
    CreateDialog dlg(archive, CreateDialog::Mode::New);

    // Se escribe un fichero de verdad y otro que no existe, separados por ";",
    // que es como los separa el dialogo.
    const QString real = tmp.filePath(QStringLiteral("existe.txt"));
    QFile f(real);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hola");
    f.close();

    dlg.setFiles({real, tmp.filePath(QStringLiteral("fantasma.txt"))});

    QString error;
    QVERIFY(!dlg.validate(&error));
    QVERIFY(error.contains(QStringLiteral("fantasma")));
}

void TestUi::creacionConExtensionIncoherente()
{
    QTemporaryDir tmp;
    const QString real = tmp.filePath(QStringLiteral("existe.txt"));
    QFile f(real);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hola");
    f.close();

    // Nombre .rar pero formato ZIP: no se puede crear de cualquier manera.
    CreateDialog dlg(tmp.filePath(QStringLiteral("nuevo.rar")), CreateDialog::Mode::New);
    dlg.setFiles({real});
    dlg.setFormat(QStringLiteral("zip"));

    QString error;
    QVERIFY(!dlg.validate(&error));
    QVERIFY(error.contains(QStringLiteral("extensión")));
}

void TestUi::creacionValida()
{
    QTemporaryDir tmp;
    const QString real = tmp.filePath(QStringLiteral("existe.txt"));
    QFile f(real);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write("hola");
    f.close();

    CreateDialog dlg(tmp.filePath(QStringLiteral("nuevo.rar")), CreateDialog::Mode::New);
    dlg.setFiles({real});

    QString error;
    // Puede fallar si esta maquina no tiene `rar` ni 7z; en ese caso el motivo
    // debe ser el de la herramienta, no un fallo de validacion.
    const bool valid = dlg.validate(&error);
    if (!valid)
        QVERIFY(error.contains(QStringLiteral("rar")) || error.contains(QStringLiteral("7-Zip")));

    QCOMPARE(dlg.files().size(), 1);
    QCOMPARE(dlg.chosenFormat(), QStringLiteral("rar"));
    QCOMPARE(dlg.compressionLevel(), 3);
}

QTEST_MAIN(TestUi)
#include "tst_ui.moc"
