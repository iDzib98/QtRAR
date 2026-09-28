#include "MainWindow.h"

#include "AddressBar.h"
#include "StatusPanel.h"
#include "ThemeManager.h"
#include "core/BinaryLocator.h"
#include "core/Diagnostics.h"
#include "core/HFormat.h"
#include "ui/dialogs/CreateDialog.h"
#include "ui/dialogs/ExtractDialog.h"
#include "ui/dialogs/LicenseDialog.h"
#include "ui/dialogs/RarSetupDialog.h"
#include "ui/dialogs/OptionsDialog.h"
#include "ui/dialogs/PasswordDialog.h"
#include "ui/VirusScan.h"
#include "ui/dialogs/ProgressDialog.h"
#include "ui/dialogs/ToolbarDialog.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QPlainTextEdit>
#include <QTemporaryFile>
#include <QMessageBox>
#include <QMimeData>
#include <QProcess>
#include <QSettings>
#include <QTemporaryDir>
#include <QStackedWidget>
#include <QSplitter>
#include <QTextBrowser>
#include <QPushButton>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QUrl>
#include <QMenuBar>
#include <QVBoxLayout>

namespace qtrar {

using enum CommandRegistry::Id;
using Id = CommandRegistry::Id;


MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), m_commands(this)
{
    setWindowIcon(ThemeManager::icon(QStringLiteral("app")));

    m_service.setTools(BinaryLocator::locate());
    connect(&m_licenseProbe, &LicenseProbe::statusChanged,
            this, &MainWindow::onLicenseStatusChanged);

    buildMenus();
    buildToolBar();
    buildCentralWidget();
    buildStatusBar();
    setupDropTarget();

    m_commands.setArchiveOpen(false);

    // Restoaurar geometria y estado de la ventana, como hace WinRAR.
    QSettings settings;
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    if (const QByteArray state = settings.value(QStringLiteral("window/state")).toByteArray();
        !state.isEmpty()) {
        restoreState(state);
    }
    m_favorites = settings.value(QStringLiteral("favorites/list")).toStringList();

    resize(940, 640);
    browseDirectory(QDir::currentPath());

    // La sonda de licencia se lanza despues de mostrar la ventana: nunca debe
    // retrasar el arranque, y su resultado solo afecta a la barra de estado.
    QTimer::singleShot(0, this, [this] {
        m_licenseProbe.probe(m_service.tools().rarPath);
    });
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// Construccion
// ---------------------------------------------------------------------------

void MainWindow::buildMenus()
{
    // El orden de cada menu se declara aqui de forma explicita, como en
    // WinRAR, en lugar de deducirlo de la lista del registro.
    const auto addAll = [this](QMenu *menu, std::initializer_list<Id> ids) {
        for (Id id : ids) {
            if (QAction *a = m_commands.action(id))
                menu->addAction(a);
        }
    };

    // --- File --------------------------------------------------------------
    QMenu *fileMenu = menuBar()->addMenu(tr("&Archivo"));
    addAll(fileMenu, {CmdOpen, CmdExtractTo, CmdExtractHere, CmdExtractSelected, CmdTest,
                      CmdView, CmdDelete, CmdFind});
    fileMenu->addSeparator();
    addAll(fileMenu, {CmdAddToArchive, CmdNewArchive, CmdRename, CmdGoto, CmdInfo, CmdComment});
    fileMenu->addSeparator();
    addAll(fileMenu, {CmdSelectAll, CmdDeselectAll, CmdInvertSelection});
    fileMenu->addSeparator();
    m_quitAction = fileMenu->addAction(tr("&Salir"), this, &QWidget::close);
    m_quitAction->setShortcut(QKeySequence::Quit);

    // --- Commands ----------------------------------------------------------
    QMenu *cmdMenu = menuBar()->addMenu(tr("&Comandos"));
    addAll(cmdMenu, {CmdExtractTo, CmdTest, CmdView, CmdDelete, CmdAddToArchive, CmdNewArchive, CmdRename, CmdComment, CmdProtect, CmdRecover, CmdEncryptNames, CmdSfx});

    // --- Tools -------------------------------------------------------------
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Herramientas"));
    addAll(toolsMenu, {CmdWizard, CmdFind, CmdInfo, CmdVirusScan, CmdGoto, CmdConfigureRar});
    toolsMenu->addSeparator();
    addAll(toolsMenu, {CmdRecover, CmdRefresh});

    // --- Favorites ---------------------------------------------------------
    m_favoritesMenu = menuBar()->addMenu(tr("&Favoritos"));
    connect(m_favoritesMenu, &QMenu::aboutToShow, this, [this] {
        m_favoritesMenu->clear();
        if (m_favorites.isEmpty()) {
            QAction *empty = m_favoritesMenu->addAction(tr("(sin favoritos)"));
            empty->setEnabled(false);
            return;
        }
        for (const QString &path : m_favorites) {
            QAction *a = m_favoritesMenu->addAction(QFileInfo(path).fileName());
            a->setData(path);
            a->setStatusTip(path);
            connect(a, &QAction::triggered, this, [this, path] { openArchive(path); });
        }
        m_favoritesMenu->addSeparator();
        connect(m_favoritesMenu->addAction(tr("Añadir actual a favoritos...")), &QAction::triggered,
                this, [this] {
                    const QString path = m_model.archivePath();
                    if (path.isEmpty() || m_favorites.contains(path))
                        return;
                    m_favorites.append(path);
                    QSettings().setValue(QStringLiteral("favorites/list"), m_favorites);
                });
    });

    // --- Options -----------------------------------------------------------
    QMenu *optMenu = menuBar()->addMenu(tr("&Opciones"));
    addAll(optMenu, {CmdOptions, CmdCustomizeToolbar});

    // --- Help --------------------------------------------------------------
    QMenu *helpMenu = menuBar()->addMenu(tr("A&yuda"));
    addAll(helpMenu, {CmdLicense, CmdHelp, CmdAbout});

    // Cada accion del registro emite una senal, en vez de un slot propio.
    connect(&m_commands, &CommandRegistry::commandTriggered, this, &MainWindow::onCommand);
}

void MainWindow::buildToolBar()
{
    m_toolBar = addToolBar(tr("Barra de herramientas"));
    m_toolBar->setObjectName(QStringLiteral("mainToolBar"));
    m_toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

    QSettings settings;
    // WinRAR: icono grande de 32 px con el texto centrado debajo. Con iconos
    // de 16 px la barra queda mas compacta, pero no se parece nada a la de alla.
    const auto style = Qt::ToolButtonStyle(
        settings.value(QStringLiteral("toolbar/style"), int(Qt::ToolButtonTextUnderIcon)).toInt());
    m_toolBar->setToolButtonStyle(style);
    const int iconSize = settings.value(QStringLiteral("toolbar/iconSize"), 32).toInt();
    m_toolBar->setIconSize(QSize(iconSize, iconSize));
    m_toolBar->setMovable(settings.value(QStringLiteral("toolbar/movable"), true).toBool());

    // WinRAR separa la barra en dos bloques: el de la izquierda (Anadir,
    // Extraer en, Comprobar, Ver, Eliminar, Buscar, Asistente, Informacion) y
    // el de la derecha, que tras la linea de separacion cambia segun haya un
    // archivo abierto: sin el, "Reparar"; con el, "Buscar virus", "Comentario",
    // "Proteger" y "Auto extraible".
    rebuildToolBarButtons();
}

void MainWindow::rebuildToolBarButtons()
{
    QSettings settings;
    // El dialogo de personalizacion guarda "toolbar/show_<id>" por boton, asi
    // que hay que releerlos en cada cambio de estado.
    const auto addGroup = [this, &settings](int group) {
        for (const CommandRegistry::Definition &d : m_commands.toolbarGroup(group, m_archiveOpen)) {
            QAction *a = m_commands.action(d.id);
            if (!a)
                continue;
            m_toolBar->addAction(a);
            a->setVisible(
                settings.value(QStringLiteral("toolbar/show_") + QString::number(int(d.id)),
                               true).toBool());
        }
    };

    // Se vacia la barra y se vuelve a montar: son unos doce botones y asi el
    // estado de "hay archivo abierto" y los ajustes nunca se desincronizan.
    m_toolBar->clear();
    addGroup(0);
    m_toolBar->addSeparator();
    addGroup(1);
}

void MainWindow::buildCentralWidget()
{
    m_stack = new QStackedWidget(this);

    // Vista de archivo.
    m_view = new QTreeView(m_stack);
    m_view->setObjectName(QStringLiteral("archiveView"));

    // El proxy debe conocer el modelo ANTES de que la vista lo use: si no,
    // el header llega sin columnas y setSectionResizeMode() sale de rango.
    // El filtro de busqueda se apoya en el mismo proxy: al filtrar no se
    // pierde la ordenacion por columnas.
    m_proxy.setSourceModel(&m_model);
    m_proxy.setSortCaseSensitivity(Qt::CaseInsensitive);
    // Las carpetas se ordenan siempre primero, como en WinRAR.
    m_proxy.setSortRole(Qt::UserRole + 1);

    m_fileSystemModel = new QFileSystemModel(this);
    m_fileSystemModel->setFilter(QDir::AllEntries | QDir::AllDirs | QDir::Files
                                 | QDir::Hidden | QDir::System);
    connect(m_fileSystemModel, &QFileSystemModel::directoryLoaded, this,
            [this](const QString &path) {
                if (m_fileSystemMode && QDir::cleanPath(path) == m_fileSystemPath)
                    updateCounts();
            });

    m_view->setModel(&m_proxy);
    m_view->setRootIsDecorated(false);
    m_view->setUniformRowHeights(true);
    m_view->setAllColumnsShowFocus(true);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_view->setSortingEnabled(true);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_view->setAlternatingRowColors(false);
    m_view->setExpandsOnDoubleClick(false);

    m_view->header()->setSectionsClickable(true);
    m_view->header()->setSortIndicatorShown(true);
    m_view->header()->setSectionResizeMode(ArchiveTreeModel::ColumnName, QHeaderView::Interactive);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnName, 300);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnSize, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnPacked, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnType, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnModified, 120);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnCrc32, 90);

    m_view->setColumnHidden(ArchiveTreeModel::ColumnType, true);

    m_addressBar = new AddressBar(this);
    m_addressBar->setEnabledAddress(false);

    auto *container = new QWidget(m_stack);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(2, 2, 2, 2);
    layout->setSpacing(2);
    layout->addWidget(m_addressBar);
    m_contentSplitter = new QSplitter(Qt::Horizontal, container);
    m_contentSplitter->setObjectName(QStringLiteral("archiveContentSplitter"));
    m_contentSplitter->addWidget(m_view);
    m_commentView = new QTextBrowser(m_contentSplitter);
    m_commentView->setObjectName(QStringLiteral("archiveCommentView"));
    m_commentView->setMinimumWidth(180);
    m_commentView->setOpenExternalLinks(true);
    m_commentView->hide();
    m_contentSplitter->addWidget(m_commentView);
    m_contentSplitter->setStretchFactor(0, 3);
    m_contentSplitter->setStretchFactor(1, 2);
    layout->addWidget(m_contentSplitter, 1);

    m_stack->addWidget(container);

    setCentralWidget(m_stack);

    connect(m_view, &QTreeView::activated, this, &MainWindow::onActivated);
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);
    connect(m_view, &QTreeView::customContextMenuRequested, this, &MainWindow::onContextMenu);
    connect(m_addressBar, &AddressBar::pathActivated, this, &MainWindow::onAddressPathActivated);
    connect(m_addressBar, &AddressBar::upRequested, this, [this] {
        if (!m_model.hasArchive()) {
            browseDirectory(QFileInfo(m_fileSystemPath).absolutePath());
        } else if (m_model.currentPath().isEmpty()) {
            browseDirectory(m_fileSystemPath);
        } else {
            m_model.setCurrentPath(m_model.parentPath());
            m_addressBar->setInternalPath(m_model.currentPath());
        }
    });
    // Atras y adelante los lleva la propia barra, que es quien guarda el
    // historial de carpetas; la ventana solo obedece `pathActivated`.
}

bool MainWindow::browseDirectory(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir() || !info.isReadable())
        return false;

    const QString clean = QDir::cleanPath(info.absoluteFilePath());
    m_fileSystemPath = clean;
    m_fileSystemMode = true;
    m_proxy.setSearchText(QString());
    m_model.setListing({});
    updateCommentView(QString());
    m_fileSystemModel->setRootPath(clean);
    configureFileSystemView();
    m_view->setRootIndex(m_proxy.mapFromSource(m_fileSystemModel->index(clean)));
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged, Qt::UniqueConnection);
    m_addressBar->setFileSystemPath(clean);
    m_addressBar->setEnabledAddress(true);
    m_extractToFolderBase = clean;
    m_stack->setCurrentWidget(m_stack->widget(0));
    updateActionStates();
    updateCounts();
    updateWindowTitle();
    statusBar()->showMessage(QDir::toNativeSeparators(clean), 0);
    return true;
}

void MainWindow::configureFileSystemView()
{
    m_proxy.setSourceModel(m_fileSystemModel);
    m_proxy.setSortRole(Qt::DisplayRole);
    m_proxy.sort(0, Qt::AscendingOrder);
    m_view->setModel(&m_proxy);
    m_view->setRootIndex({});
    m_view->setColumnHidden(2, false); // Tipo
    m_view->setColumnHidden(3, false); // Modificado
    m_view->setColumnHidden(4, true);
    m_view->setColumnHidden(5, true);
    m_view->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_view->header()->resizeSection(0, 360);
    m_view->header()->resizeSection(1, 100);
    m_view->header()->resizeSection(2, 140);
    m_view->header()->resizeSection(3, 170);
}

void MainWindow::configureArchiveView()
{
    m_proxy.setSourceModel(&m_model);
    m_proxy.setSortRole(Qt::UserRole + 1);
    m_view->setModel(&m_proxy);
    m_view->setRootIndex({});
    m_view->setColumnHidden(ArchiveTreeModel::ColumnPacked, false);
    m_view->setColumnHidden(ArchiveTreeModel::ColumnType, true);
    m_view->setColumnHidden(ArchiveTreeModel::ColumnModified, false);
    m_view->setColumnHidden(ArchiveTreeModel::ColumnCrc32, false);
    m_view->header()->setSectionResizeMode(ArchiveTreeModel::ColumnName, QHeaderView::Interactive);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnName, 300);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnSize, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnPacked, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnType, 90);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnModified, 120);
    m_view->header()->resizeSection(ArchiveTreeModel::ColumnCrc32, 90);
}

void MainWindow::buildStatusBar()
{
    m_statusPanel = new StatusPanel(this);
    statusBar()->addPermanentWidget(m_statusPanel, 1);
    statusBar()->showMessage(tr("Listo"), 0);
}

void MainWindow::setupDropTarget()
{
    setAcceptDrops(true);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    // Solo se acepta si hay al menos un archivo suelto: cualquier otra cosa
    // (un texto, una imagen) haria ruido con el cursor de prohibido.
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty())
        event->acceptProposedAction();
    else
        event->ignore();
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls() && !event->mimeData()->urls().isEmpty())
        event->acceptProposedAction();
    else
        event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasUrls())
        return;

    // WinRAR abre lo que se le suelta. Se coge el primer archivo y se avisa del
    // resto en vez de descartarlos en silencio.
    const QList<QUrl> urls = event->mimeData()->urls();
    QStringList archives;
    for (const QUrl &url : urls) {
        const QString path = url.toLocalFile();
        if (!path.isEmpty() && ArchiveService::detectFormat(path) != ArchiveFormat::Unknown)
            archives.append(path);
    }

    if (archives.isEmpty()) {
        event->ignore();
        return;
    }

    event->acceptProposedAction();
    openArchive(archives.first());

    if (archives.size() > 1) {
        statusBar()->showMessage(
            tr("Se ha abierto %1. Usa la ventana para abrir los demas: %2")
                .arg(archives.first(), QString::number(archives.size() - 1)),
            8000);
    }
}

// ---------------------------------------------------------------------------
// Apertura y navegacion
// ---------------------------------------------------------------------------

bool MainWindow::openArchive(const QString &givenPath, bool asCommandLine)
{
    Q_UNUSED(asCommandLine)

    // Si se abre una parte que no es la primera, se sube a `part1`, que es por
    // donde se abre una serie, igual que hace WinRAR.
    const QString path = m_service.firstVolumeOf(givenPath);
    const QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("No se encuentra el archivo:\n%1").arg(path));
        return false;
    }

    const ArchiveFormat fmt = ArchiveService::detectFormat(path);
    if (fmt == ArchiveFormat::Unknown) {
        QMessageBox::warning(this, tr("Error"),
                             tr("No es un archivo de los formatos soportados.\n\n%1").arg(path));
        return false;
    }
    if (fmt == ArchiveFormat::Zip && m_service.tools().sevenZipPath.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
                             tr("Es un archivo ZIP y 7-Zip no esta disponible en el sistema.\n"
                                "Los binarios de RAR no pueden leer ZIP."));
        return false;
    }

    // En una serie de volumenes, listar la primera parte puede "ir bien" y aun
    // asi faltar un volumen intermedio. Se pregunta antes de mostrar nada, que
    // es lo que hace WinRAR, porque si no el usuario ve un listado recortado y
    // no se entera hasta que falla al extraer.
    if (fmt == ArchiveFormat::Rar) {
        QString missingVolume;
        if (!m_service.volumesPresent(path, &missingVolume)) {
            QMessageBox::warning(
                this, tr("Error"),
                tr("Falta un volumen de la serie:\n%1\n\n"
                   "Coloca todos los volumenes en la misma carpeta y vuelve a intentarlo.")
                    .arg(missingVolume));
            return false;
        }
    }

    m_progress = nullptr;
    const QString previousPassword = m_password;
    const bool previousHasPassword = m_hasPassword;
    m_password.clear();
    m_hasPassword = false;

    // La carga puede tardar en archivos solidos o de red: se hace sin bloquear
    // la ventana, mostrando la barra de estado.
    statusBar()->showMessage(tr("Leyendo %1...").arg(fi.fileName()));

    ProcessOutcome outcome;
    ArchiveListing listing = m_service.list(path, m_password, m_hasPassword, &outcome);

    if (outcome.needsPassword()) {
        PasswordDialog dlg(this);
        dlg.setReason(tr("El archivo tiene las cabeceras cifradas. Introduce la contraseña."));
        while (dlg.exec() == QDialog::Accepted) {
            m_password = dlg.password();
            m_hasPassword = true;
            listing = m_service.list(path, m_password, m_hasPassword, &outcome);
            if (!outcome.needsPassword())
                break;
            dlg.setReason(tr("Contraseña incorrecta. Inténtalo de nuevo."));
        }
    }

    if (!outcome.success) {
        m_password = previousPassword;
        m_hasPassword = previousHasPassword;
        reportError(outcome);
        return false;
    }

    if (fmt == ArchiveFormat::Rar)
        listing.info.comment = m_service.readComment(path);
    m_model.setListing(listing);
    updateCommentView(listing.info.comment);
    m_proxy.setSearchText(QString());
    m_fileSystemPath = fi.absolutePath();
    m_extractToFolderBase = fi.absolutePath();
    m_fileSystemMode = false;
    configureArchiveView();
    connect(m_view->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged, Qt::UniqueConnection);
    m_stack->setCurrentWidget(m_stack->widget(0));
    m_addressBar->setArchivePath(path, QString());
    m_addressBar->setEnabledAddress(true);
    updateActionStates();
    refreshView();
    updateWindowTitle();
    statusBar()->showMessage(tr("Listo"), 0);
    return true;
}

namespace {

/// Atajo para los fallos que son "falta la herramienta", sin montar a mano el
/// ProcessOutcome completo en cada sitio.
ProcessOutcome toolMissing(const QString &why)
{
    ProcessOutcome outcome;
    outcome.success = false;
    outcome.diagnostic = Diagnostic::ToolNotFound;
    outcome.exitCode = ExitCode::CommandLineError;
    outcome.message = why;
    return outcome;
}

/// Bytes en unidades legibles, como las muestra WinRAR: 1.234.567 bytes.
QString humanSize(qint64 bytes)
{
    if (bytes < 0)
        return QCoreApplication::translate("qtrar::MainWindow", "desconocido");
    static const char *units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = double(bytes);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        ++unit;
    }
    return unit == 0 ? QStringLiteral("%1 %2").arg(bytes).arg(QLatin1String(units[unit]))
                     : QStringLiteral("%1 %2")
                           .arg(value, 0, 'f', value < 10.0 ? 2 : 1)
                           .arg(QLatin1String(units[unit]));
}

} // namespace

void MainWindow::showCommentEditor()
{
    if (m_model.info().format.startsWith(QLatin1String("ZIP"))) {
        QMessageBox::information(
            this, tr("Comentario"),
            tr("Un ZIP no admite comentario. El equivalente es el fichero de texto que hay "
               "dentro del archivo."));
        return;
    }
    if (m_service.tools().rarPath.isEmpty()) {
        reportError(toolMissing(tr("Guardar el comentario necesita el programa 'rar'.")));
        return;
    }

    // El comentario vive en la cabecera del archivo, asi que solo se puede
    // escribir con `rar`. Un ZIP tendria que llevar el texto como fichero.
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Comentario del archivo"));
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(
        tr("Este texto se guarda dentro del archivo y se ve al abrirlo con WinRAR."), &dialog));
    auto *editor = new QPlainTextEdit(&dialog);
    editor->setObjectName(QStringLiteral("commentEditor"));
    editor->setPlainText(m_service.readComment(m_model.archivePath()));
    editor->setMinimumSize(480, 220);
    layout->addWidget(editor);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Guardar"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted)
        return;

    // `rar c -z` solo admite un fichero: se escribe a uno temporal y se pasa.
    // Es el unico modo que tiene RAR de poner texto en la cabecera.
    QTemporaryFile commentFile(this);
    if (!commentFile.open()) {
        QMessageBox::warning(this, tr("Error"), tr("No se pudo crear el fichero temporal."));
        return;
    }
    const QString savedComment = editor->toPlainText();
    const QByteArray text = savedComment.toUtf8();
    if (commentFile.write(text) < 0) {
        QMessageBox::warning(this, tr("Error"), tr("No se pudo escribir el comentario."));
        return;
    }
    commentFile.close();

    const QString archive = m_model.archivePath();
    if (runOperation(tr("Guardando el comentario..."), 1,
                     [this, archive, &commentFile](ProgressDialog *, bool *) {
                         return m_service.setComment(archive, commentFile.fileName());
                     })) {
        // Reflejar inmediatamente lo que acaba de guardar el usuario; la
        // lectura de cabecera al volver a abrir el archivo confirma lo persistido.
        updateCommentView(savedComment);
    }
}

void MainWindow::showHelp()
{
    // Ayuda de un vistazo, no un manual: lo que hace falta saber para empezar.
    QMessageBox box(this);
    box.setWindowTitle(tr("Ayuda de QtRAR"));
    box.setIcon(QMessageBox::Information);
    box.setText(tr("<b>QtRAR</b> — gestor de archivos RAR y ZIP"));
    box.setInformativeText(
        tr("<b>Para empezar</b><ul>"
           "<li><b>Extraer</b> extrae el archivo entero o lo que tengas seleccionado, "
           "a la carpeta que elijas.</li>"
           "<li><b>Probar</b> verifica que el archivo no está dañado y que están "
           "todos los volúmenes.</li>"
           "<li><b>Añadir</b> y <b>Nuevo archivo</b> crean RAR o ZIP.</li>"
           "<li><b>Buscar</b> localiza ficheros por nombre dentro del archivo, "
           "aunque estén en subcarpetas.</li>"
           "</ul>"
           "<b>Atajos</b><ul>"
           "<li><b>Ctrl+O</b> abrir &nbsp; <b>Ctrl+F</b> buscar &nbsp; "
           "<b>Ctrl+A</b> seleccionar todo &nbsp; <b>Alt+Left</b> atrás</li>"
           "<li><b>Delete</b> borrar la selección &nbsp; <b>F2</b> renombrar</li>"
           "</ul>"
           "<b>Programas que usa</b><br>"
           "RAR y ZIP los leen y los escriben los programas oficiales de RARLAB y "
           "7-Zip. Si falta alguno, QtRAR avisa en la barra de abajo en vez de fallar "
           "en silencio."));
    box.exec();
}

void MainWindow::showLicenseDialog()
{
    LicenseDialog dialog(m_licenseStatus, this);
    connect(&dialog, &LicenseDialog::recheckRequested, &dialog, [this, &dialog] {
        dialog.setStatus(m_licenseProbe.probe(m_service.tools().rarPath));
    });
    dialog.exec();
}

bool MainWindow::configureRarBinary()
{
    RarSetupDialog dialog(m_service.tools().rarPath, this);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    QSettings settings;
    settings.setValue(QStringLiteral("tools/rarPath"), dialog.binaryPath());
    settings.sync();

    const ExternalTools tools = BinaryLocator::locate();
    if (tools.rarPath.isEmpty()) {
        QMessageBox::warning(this, tr("RAR no disponible"),
                             tr("No se pudo ejecutar el binario RAR seleccionado."));
        return false;
    }
    m_service.setTools(tools);
    m_licenseProbe.probe(tools.rarPath);
    statusBar()->showMessage(tr("Binario RAR configurado: %1").arg(tools.rarPath), 6000);
    return true;
}

void MainWindow::createArchive(const QString &suggestedPath, const QStringList &initialFiles)
{
    CreateDialog dlg(suggestedPath,
                     suggestedPath.isEmpty() ? CreateDialog::Mode::New
                                             : CreateDialog::Mode::Add,
                     this);
    if (!initialFiles.isEmpty())
        dlg.setFiles(initialFiles);
    if (dlg.exec() != QDialog::Accepted)
        return;

    CreateOptions o;
    o.archivePath = dlg.archivePath();
    o.files = dlg.files();
    o.compressionLevel = dlg.compressionLevel();
    o.solid = dlg.solid();
    o.encryptHeaders = dlg.encryptHeaders();
    if (dlg.hasPassword()) {
        o.hasPassword = true;
        o.password = dlg.password();
    }

    runOperation(tr("Creando %1...").arg(QFileInfo(o.archivePath).fileName()),
                 o.files.size(),
                 [this, o](ProgressDialog *, bool *) { return m_service.create(o); });

    // Crear y abrirlo a la vez es lo que espera la gente: si el archivo se ha
    // creado, la ventana debe mostrar su contenido, no seguir vacia.
    if (QFileInfo::exists(o.archivePath))
        openArchive(o.archivePath);
}

void MainWindow::showArchiveInfo()
{
    const ArchiveInfo info = m_model.info();
    if (!info.isValid())
        return;

    QStringList rows;
    auto row = [&rows](const QString &key, const QString &value) {
        rows << QStringLiteral("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(key, value);
    };

    row(tr("Nombre"), QFileInfo(info.path).fileName());
    row(tr("Formato"), info.format);
    row(tr("Ubicación"), QFileInfo(info.path).absolutePath());
    row(tr("Tamaño del archivo"), humanSize(info.compressedSize));
    row(tr("Tamaño descomprimido"), humanSize(info.uncompressedSize));
    row(tr("Ficheros"), QString::number(info.fileCount));
    row(tr("Carpetas"), QString::number(info.dirCount));
    row(tr("Cifrado"),
        info.anyEncrypted ? tr("Sí, algunos miembros están cifrados") : tr("No"));
    row(tr("Cabecera cifrada"), info.encryptedHeaders ? tr("Sí") : tr("No"));
    row(tr("Sólido"), info.solid ? tr("Sí") : tr("No"));
    if (info.volume) {
        row(tr("Volumen"),
            info.volumeNumber >= 0 ? tr("Parte %1").arg(info.volumeNumber) : tr("Sí"));
    }

    QMessageBox box(this);
    box.setWindowTitle(tr("Información de %1").arg(QFileInfo(info.path).fileName()));
    box.setIcon(QMessageBox::Information);
    box.setText(QStringLiteral("<h3>%1</h3>").arg(QFileInfo(info.path).fileName()));
    box.setInformativeText(QStringLiteral("<table cellspacing=\"4\">%1</table>")
                               .arg(rows.join(QString())));

    // El comentario va aparte, en el desplegable de detalle: es texto largo y
    // no cabe en una fila de la tabla.
    const QString comment = m_service.readComment(info.path);
    if (!comment.isEmpty())
        box.setDetailedText(comment);

    box.exec();
}

void MainWindow::showAboutDialog()
{
    const ExternalTools tools = m_service.tools();
    QMessageBox::about(
        this, tr("Acerca de QtRAR"),
        tr("<b>QtRAR %1</b><br><br>"
           "Gestor de archivos con la interfaz de WinRAR, construido sobre "
           "los binarios oficiales de RAR para Linux.<br><br>"
           "unrar %2 &nbsp; rar %3 &nbsp; 7-Zip: %4")
            .arg(QStringLiteral(QTRAR_VERSION), tools.unrarVersion, tools.rarVersion,
                 tools.sevenZipPath.isEmpty() ? tr("no disponible")
                                              : QFileInfo(tools.sevenZipPath).fileName()));
}

void MainWindow::openSecondaryInstanceRequest(const QString &path)
{
    if (path.isEmpty())
        return;
    show();
    raise();
    activateWindow();
    openArchive(path);
}

void MainWindow::refreshView()
{
    if (m_fileSystemMode) {
        m_fileSystemModel->setRootPath(m_fileSystemPath);
        m_view->setRootIndex(m_proxy.mapFromSource(m_fileSystemModel->index(m_fileSystemPath)));
        updateCounts();
        return;
    }
    m_model.refresh();
    updateCounts();
    updateActionStates();
}

void MainWindow::onActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    if (m_fileSystemMode) {
        const QModelIndex src = m_proxy.mapToSource(index);
        const QString path = m_fileSystemModel->filePath(src);
        const QFileInfo info(path);
        if (m_fileSystemModel->fileName(src) == QLatin1String("."))
            return;
        if (m_fileSystemModel->fileName(src) == QLatin1String("..")) {
            browseDirectory(QFileInfo(m_fileSystemPath).absolutePath());
            return;
        }
        if (info.isDir()) {
            browseDirectory(path);
            return;
        }
        if (ArchiveService::detectFormat(path) != ArchiveFormat::Unknown) {
            openArchive(path);
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
        return;
    }
    const QModelIndex src = m_proxy.mapToSource(index);
    if (m_model.isParentEntry(src)) {
        if (m_model.currentPath().isEmpty()) {
            browseDirectory(m_fileSystemPath);
        } else {
            m_model.setCurrentPath(m_model.parentPath());
            m_addressBar->setInternalPath(m_model.currentPath());
            updateCounts();
        }
        return;
    }
    if (m_model.isDirectory(src)) {
        m_model.setCurrentPath(m_model.entryPath(src));
        m_addressBar->setInternalPath(m_model.currentPath());
        return;
    }
    // Doble clic en un fichero: WinRAR lo abre con la aplicacion asociada.
    onCommand(CommandRegistry::CmdView);
}

void MainWindow::goTo(const QString &internalPath)
{
    onAddressPathActivated(internalPath);
}

void MainWindow::onAddressPathActivated(const QString &path)
{
    if (m_fileSystemMode) {
        browseDirectory(path);
        return;
    }
    m_model.setCurrentPath(path);
    m_addressBar->setInternalPath(m_model.currentPath());
}

void MainWindow::updateWindowTitle()
{
    const QString path = m_model.archivePath();
    if (path.isEmpty()) {
        setWindowTitle(m_fileSystemPath.isEmpty()
                           ? QStringLiteral("QtRAR")
                           : tr("%1 - QtRAR").arg(QDir::toNativeSeparators(m_fileSystemPath)));
        return;
    }
    const ArchiveInfo &info = m_model.info();
    setWindowTitle(tr("%1 - QtRAR").arg(QFileInfo(path).fileName()));
    Q_UNUSED(info)
}

void MainWindow::updateCounts()
{
    if (!m_model.hasArchive()) {
        if (m_fileSystemMode && m_fileSystemModel && m_statusPanel) {
            const QModelIndex root = m_fileSystemModel->index(m_fileSystemPath);
            int files = 0;
            int folders = 0;
            for (int row = 0; row < m_fileSystemModel->rowCount(root); ++row) {
                const QModelIndex entry = m_fileSystemModel->index(row, 0, root);
                if (m_fileSystemModel->fileName(entry) == QLatin1String("..")
                    || m_fileSystemModel->fileName(entry) == QLatin1String("."))
                    continue;
                const QFileInfo info = m_fileSystemModel->fileInfo(entry);
                info.isDir() ? ++folders : ++files;
            }
            m_statusPanel->setCounts(files, folders, -1, -1);
            return;
        }
        m_statusPanel->setCounts(0, 0, -1, -1);
        return;
    }
    const ArchiveInfo &info = m_model.info();
    m_statusPanel->setCounts(info.fileCount, info.dirCount, info.totalSize, info.totalPacked);
}

void MainWindow::updateCommentView(const QString &comment)
{
    if (!m_commentView)
        return;
    m_commentView->setPlainText(comment);
    m_commentView->setVisible(!comment.trimmed().isEmpty());
    if (!comment.trimmed().isEmpty() && m_contentSplitter) {
        const int total = qMax(1, m_contentSplitter->width());
        m_contentSplitter->setSizes({total * 2 / 3, total / 3});
    }
}

QString MainWindow::defaultExtractionDestination() const
{
    const QString configured = QSettings().value(QStringLiteral("paths/extractTo")).toString().trimmed();
    if (!configured.isEmpty())
        return configured;
    if (!m_model.archivePath().isEmpty())
        return QFileInfo(m_model.archivePath()).absolutePath();
    return m_fileSystemPath.isEmpty() ? QDir::homePath() : m_fileSystemPath;
}

void MainWindow::updateActionStates()
{
    const bool hasArchive = m_model.hasArchive();
    m_commands.setArchiveOpen(hasArchive);
    // El bloque de la derecha de la barra depende de esto: sin archivo sale
    // "Reparar", con archivo salen "Buscar virus", "Comentario", "Proteger" y
    // "Auto extraible". Solo se remonta cuando el estado cambia de verdad,
    // porque `updateActionStates` se llama en cada refresco de la vista.
    if (m_archiveOpen != hasArchive) {
        m_archiveOpen = hasArchive;
        rebuildToolBarButtons();
    }
    if (QAction *a = m_commands.action(CommandRegistry::CmdUp))
        a->setEnabled(!m_model.currentPath().isEmpty());
}

void MainWindow::onSelectionChanged()
{
    if (m_fileSystemMode) {
        const QModelIndexList rows = m_view->selectionModel()->selectedRows();
        if (rows.isEmpty()) {
            m_statusPanel->setSelectionInfo(QString());
            return;
        }
        qint64 size = 0;
        for (const QModelIndex &idx : rows) {
            const QFileInfo info(m_fileSystemModel->filePath(m_proxy.mapToSource(idx)));
            if (info.isFile())
                size += info.size();
        }
        m_statusPanel->setSelectionInfo(tr("%1 seleccionados (%2)")
                                            .arg(rows.size())
                                            .arg(HFormat::fileSize(size)));
        return;
    }
    const int n = m_view->selectionModel()->selectedRows().size();
    if (n <= 1) {
        m_statusPanel->setSelectionInfo(QString());
        return;
    }
    qint64 size = 0;
    for (const QModelIndex &idx : m_view->selectionModel()->selectedRows()) {
        const ArchiveEntry *e = m_model.entryAt(m_proxy.mapToSource(idx));
        if (e && e->size > 0)
            size += e->size;
    }
    m_statusPanel->setSelectionInfo(tr("%1 seleccionados (%2)")
                                        .arg(n)
                                        .arg(HFormat::fileSize(size)));
}

QStringList MainWindow::selectedPaths() const
{
    QStringList paths;
    for (const QModelIndex &idx : m_view->selectionModel()->selectedRows()) {
        if (m_fileSystemMode) {
            const QModelIndex source = m_proxy.mapToSource(idx);
            if (m_fileSystemModel->fileName(source) == QLatin1String("..")
                || m_fileSystemModel->fileName(source) == QLatin1String("."))
                continue;
            const QString p = m_fileSystemModel->filePath(source);
            if (!p.isEmpty())
                paths.append(p);
            continue;
        }
        const QModelIndex source = m_proxy.mapToSource(idx);
        if (m_model.isParentEntry(source))
            continue;
        const QString p = m_model.entryPath(source);
        if (!p.isEmpty())
            paths.append(p);
    }
    return paths;
}

bool MainWindow::confirmExtractionOverwrites(ExtractOptions &options)
{
    if (options.existingFiles != ExistingFilesMode::Confirm)
        return true;

    QStringList collisions;
    for (const ArchiveEntry &entry : m_model.listing().entries) {
        if (entry.isDir())
            continue;
        if (!options.items.isEmpty()) {
            bool selected = false;
            for (const QString &item : options.items) {
                if (entry.name == item || entry.name.startsWith(item + QLatin1Char('/'))) {
                    selected = true;
                    break;
                }
            }
            if (!selected)
                continue;
        }
        const QString relative = options.preservePaths
            ? entry.name : QFileInfo(entry.name).fileName();
        if (QFileInfo::exists(QDir(options.destination).filePath(relative)))
            collisions.append(relative);
    }

    if (collisions.isEmpty()) {
        options.existingFiles = ExistingFilesMode::Overwrite;
        return true;
    }

    QString examples = collisions.mid(0, 4).join(QLatin1Char('\n'));
    if (collisions.size() > 4)
        examples += tr("\n... y %1 más").arg(collisions.size() - 4);
    const auto answer = QMessageBox::question(
        this, tr("Confirmar sobrescritura"),
        tr("Ya existen %1 archivos en el destino:\n\n%2\n\n¿Quieres sobrescribirlos?")
            .arg(collisions.size()).arg(examples),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::No);
    if (answer == QMessageBox::Cancel)
        return false;
    options.existingFiles = answer == QMessageBox::Yes ? ExistingFilesMode::Overwrite
                                                        : ExistingFilesMode::Skip;
    return true;
}

// ---------------------------------------------------------------------------
// Acciones
// ---------------------------------------------------------------------------

void MainWindow::onCommand(CommandRegistry::Id id)
{
    switch (id) {
    case CommandRegistry::CmdAbout:
        showAboutDialog();
        return;
    case CommandRegistry::CmdOpen: {
        const QString path = QFileDialog::getOpenFileName(
            this, tr("Abrir archivo"), m_extractToFolderBase,
            tr("Archivos RAR (*.rar);;Archivos ZIP (*.zip);;Todos los archivos (*)"));
        if (!path.isEmpty()) {
            m_extractToFolderBase = QFileInfo(path).absolutePath();
            openArchive(path);
        }
        break;
    }

    case CommandRegistry::CmdExtractTo:
    case CommandRegistry::CmdExtractHere:
    case CommandRegistry::CmdExtractSelected: {
        if (!m_model.hasArchive())
            break;
        ExtractDialog dlg(this);
        dlg.setArchiveName(QFileInfo(m_model.archivePath()).fileName());

        QStringList items;
        QString base = defaultExtractionDestination();
        if (id == CommandRegistry::CmdExtractHere) {
            const QModelIndex idx = m_view->currentIndex();
            if (idx.isValid()) {
                const QModelIndex source = m_proxy.mapToSource(idx);
                if (m_model.isParentEntry(source))
                    break;
                const QString p = m_model.entryPath(source);
                if (m_model.isDirectory(source)
                    && p == m_model.currentPath()) {
                    base = QFileInfo(m_model.archivePath()).absolutePath();
                    items << (p.isEmpty() ? QString() : p + QStringLiteral("/*"));
                } else {
                    items << p;
                }
            }
        } else if (id == CommandRegistry::CmdExtractSelected) {
            items = selectedPaths();
            if (items.isEmpty())
                break;
        }
        dlg.setSelectedPaths(items);
        dlg.setSelectionCount(items.isEmpty() ? 0 : items.size(), 0);
        dlg.setDefaultDestination(base);

        if (dlg.exec() != QDialog::Accepted)
            break;

        if (dlg.hasPassword()) {
            m_password = dlg.password();
            m_hasPassword = true;
        }
        ExtractOptions options = dlg.options(m_hasPassword, m_password);
        if (!confirmExtractionOverwrites(options))
            break;
        if (runOperation(tr("Extrayendo..."), options.items.isEmpty() ? m_model.info().fileCount
                                                                      : options.items.size(),
                         [this, options](ProgressDialog *dlg, bool *cancelled) {
                             Q_UNUSED(dlg)
                             *cancelled = false;
                             return m_service.extract(m_model.archivePath(), options);
                         }) && dlg.showInExplorer()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(options.destination));
        }
        break;
    }

    case CommandRegistry::CmdTest: {
        if (!m_model.hasArchive())
            break;
        runOperation(tr("Probando..."), m_model.info().fileCount,
                     [this](ProgressDialog *, bool *cancelled) {
                         *cancelled = false;
                         return m_service.test(m_model.archivePath(), QStringList(), m_password,
                                               m_hasPassword);
                     });
        break;
    }

    case CommandRegistry::CmdView:
    case CommandRegistry::CmdOpenFile: {
        // Abrir con la aplicacion asociada: WinRAR extrae a un temporal y
        // lanza el visor. Es el unico punto donde se toca el disco.
        const QStringList sel = selectedPaths();
        if (sel.isEmpty() || sel.size() > 1) {
            QMessageBox::information(this, tr("Ver"),
                                     tr("Selecciona un fichero para abrirlo."));
            break;
        }
        const QString name = sel.first();
        auto tmp = std::make_unique<QTemporaryDir>();
        if (!tmp->isValid()) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("No se pudo crear la carpeta temporal."));
            break;
        }
        ExtractOptions o;
        o.destination = tmp->path();
        o.items = {name};
        o.hasPassword = m_hasPassword;
        o.password = m_password;
        const ProcessOutcome r = m_service.extract(m_model.archivePath(), o);
        if (!r.success) {
            reportError(r);
            break;
        }
        const QString file = QDir(tmp->path()).absoluteFilePath(name);
        if (!QFileInfo::exists(file)) {
            QMessageBox::warning(this, tr("Error"),
                                 tr("No se pudo extraer %1.").arg(name));
            break;
        }
        // QDesktopServices abre otra aplicacion de forma asincrona. Si `tmp`
        // se destruye al salir de este bloque, xdg-open recibe una ruta que ya
        // no existe. Guardar el directorio hasta que cierre QtRAR mantiene el
        // fichero accesible durante toda la apertura del visor.
        m_viewTemps.push_back(std::move(tmp));
        if (!QDesktopServices::openUrl(QUrl::fromLocalFile(file))) {
            m_viewTemps.pop_back();
            QMessageBox::warning(this, tr("Error"),
                                 tr("No se pudo abrir %1 con la aplicación asociada.")
                                     .arg(QFileInfo(file).fileName()));
        }
        break;
    }

    case CommandRegistry::CmdDelete: {
        const QStringList sel = selectedPaths();
        if (sel.isEmpty())
            break;

        const QString what = sel.size() == 1 ? tr("«%1»").arg(QFileInfo(sel.first()).fileName())
                                             : tr("los %1 elementos seleccionados").arg(sel.size());
        const QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Eliminar"),
            tr("¿Seguro que quieres eliminar %1 del archivo?\n\n"
               "Se modificará el archivo de forma permanente.").arg(what),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (answer != QMessageBox::Yes)
            break;

        // Borrar exige el binario `rar`: `unrar` 7.23 no tiene ese comando, y
        // un ZIP solo se puede reescribir con 7-Zip.
        if (m_service.tools().rarPath.isEmpty()) {
            reportError(toolMissing(tr("Modificar un archivo RAR necesita el programa 'rar'.")));
            break;
        }
        if (m_model.info().format.startsWith(QLatin1String("ZIP"))) {
            reportError(toolMissing(tr("Modificar un ZIP no está soportado: habría que "
                                       "reescribirlo entero con 7-Zip y rehacer las rutas.")));
            break;
        }

        const QString archive = m_model.archivePath();
        runOperation(tr("Eliminando..."), sel.size(),
                     [this, archive, sel](ProgressDialog *, bool *) {
                         return m_service.removeItems(archive, sel);
                     });
        break;
    }

    case CommandRegistry::CmdRename: {
        const QStringList sel = selectedPaths();
        if (sel.size() != 1) {
            if (!sel.isEmpty())
                QMessageBox::information(this, tr("Renombrar"),
                                         tr("Solo se puede renombrar un elemento a la vez."));
            break;
        }

        const QString current = sel.first();
        bool accepted = false;
        const QString typed = QInputDialog::getText(
            this, tr("Renombrar"), tr("Nuevo nombre:"), QLineEdit::Normal,
            QFileInfo(current).fileName(), &accepted);
        if (!accepted || typed.trimmed().isEmpty())
            break;

        // Renombrar conserva la carpeta: solo cambia el ultimo componente.
        const QString parent = current.left(current.lastIndexOf(QLatin1Char('/')) + 1);
        const QString target = parent + typed.trimmed();

        if (m_service.tools().rarPath.isEmpty()) {
            reportError(toolMissing(tr("Modificar un archivo RAR necesita el programa 'rar'.")));
            break;
        }

        const QString archive = m_model.archivePath();
        runOperation(tr("Renombrando..."), 1,
                     [this, archive, current, target](ProgressDialog *, bool *) {
                         return m_service.renameItem(archive, current, target);
                     });
        break;
    }

    case CommandRegistry::CmdNewArchive:
        createArchive(QString());
        break;

    case CommandRegistry::CmdAddToArchive: {
        // El botón permanece activo aunque falte el motor de escritura: permite
        // instalar/seleccionar el `rar` oficial en vez de dejar al usuario ante
        // una acción apagada que no explica cómo continuar.
        if (m_service.tools().rarPath.isEmpty() && !configureRarBinary())
            break;
        // Si hay un archivo abierto se ofrece ese nombre; si no, se propone uno
        // junto a lo que se haya seleccionado, que es lo que hace WinRAR.
        QString suggested = m_model.archivePath();
        if (suggested.isEmpty()) {
            const QStringList sel = selectedPaths();
            if (!sel.isEmpty())
                suggested = sel.first() + QStringLiteral(".rar");
        }
        createArchive(suggested, m_fileSystemMode ? selectedPaths() : QStringList{});
        break;
    }

    case CommandRegistry::CmdComment:
        showCommentEditor();
        break;

    case CommandRegistry::CmdOptions: {
        OptionsDialog dlg(this);
        if (dlg.exec() == QDialog::Accepted && dlg.apply()) {
            QMessageBox::information(this, tr("Opciones"),
                                     tr("Para que el idioma cambie hay que reiniciar QtRAR."));
        }
        break;
    }

    case CommandRegistry::CmdCustomizeToolbar: {
        ToolbarDialog dlg(&m_commands, this);
        if (dlg.exec() != QDialog::Accepted)
            break;
        dlg.apply();
        // Los cambios se aplican a las acciones, pero el estilo de los botones
        // y si la barra se puede mover son cosas de la propia barra.
        QSettings settings;
        m_toolBar->setToolButtonStyle(
            Qt::ToolButtonStyle(settings.value(QStringLiteral("toolbar/style"),
                                               int(Qt::ToolButtonTextUnderIcon)).toInt()));
        m_toolBar->setMovable(
            settings.value(QStringLiteral("toolbar/movable"), true).toBool());
        const int iconSize = settings.value(QStringLiteral("toolbar/iconSize"), 32).toInt();
        m_toolBar->setIconSize(QSize(iconSize, iconSize));
        break;
    }

    case CommandRegistry::CmdHelp:
        showHelp();
        break;

    case CommandRegistry::CmdLicense:
        showLicenseDialog();
        break;

    case CommandRegistry::CmdConfigureRar:
        configureRarBinary();
        break;

    case CommandRegistry::CmdInfo:
        showArchiveInfo();
        break;

    case CommandRegistry::CmdFind: {
        // WinRAR filtra en la barra de direcciones. Se pide el texto con un
        // dialogo pequeno, se aplica al proxy y se avisa de cuantos han
        // quedado a la vista.
        bool accepted = false;
        const QString typed = QInputDialog::getText(
            this, tr("Buscar"),
            tr("Buscar en el archivo (se admiten * y ?, p. ej. *.txt):"), QLineEdit::Normal,
            m_proxy.searchText(), &accepted);
        if (!accepted)
            break;

        // El modelo pasa a modo busqueda: deja de mostrar una carpeta y lista
        // a plano todo lo que coincida, este donde este dentro del archivo.
        // El proxy despues vuelve a filtrar, que es barato y deja la logica
        // de los comodines en un solo sitio.
        m_proxy.setSearchText(typed);
        m_model.setSearchPattern(typed, [&typed](const QString &name) {
            return FindProxyModel::matchesPattern(name, typed);
        });

        if (typed.isEmpty()) {
            statusBar()->showMessage(tr("Búsqueda cancelada."), 3000);
            break;
        }
        const int shown = m_proxy.rowCount();
        statusBar()->showMessage(
            shown == 0 ? tr("No hay coincidencias para «%1».").arg(typed)
                       : tr("%1 de %2 entradas coinciden con «%3».")
                             .arg(shown).arg(m_model.listing().entries.size()).arg(typed),
            6000);
        break;
    }

    case CommandRegistry::CmdBack:
        m_addressBar->goBack();
        break;

    case CommandRegistry::CmdRefresh:
        refreshView();
        break;

    case CommandRegistry::CmdSelectAll:
        m_view->selectAll();
        break;
    case CommandRegistry::CmdDeselectAll:
        m_view->clearSelection();
        break;
    case CommandRegistry::CmdInvertSelection: {
        // Seleccionar todo menos lo ya seleccionado.
        QItemSelection toggle;
        for (int row = 0; row < m_proxy.rowCount(); ++row) {
            const QModelIndex idx = m_proxy.index(row, 0);
            if (!m_view->selectionModel()->isSelected(idx))
                toggle.select(idx, idx);
        }
        m_view->selectionModel()->select(toggle, QItemSelectionModel::Toggle);
        break;
    }

    case CommandRegistry::CmdGoto: {
        // "Ir a" es el nombre interno exacto, como en WinRAR: no siempre es
        // una carpeta, un fichero vale igual.
        bool ok = false;
        const QString path = QInputDialog::getText(
            this, tr("Ir a"), tr("Nombre dentro del archivo (usa / para las carpetas):"),
            QLineEdit::Normal, m_model.currentPath(), &ok);
        if (ok && !path.isEmpty())
            goTo(path);
        break;
    }
    case CommandRegistry::CmdWizard: {
        // El asistente de WinRAR es una extraccion guiada: destino y nombre.
        QString destination;
        bool showInExplorer = false;
        const bool extracted = runOperation(tr("Asistente"), 1,
                                             [this, &destination, &showInExplorer](ProgressDialog *, bool *) {
            ExtractDialog dlg(this);
            dlg.setArchiveName(QFileInfo(m_model.archivePath()).completeBaseName());
            dlg.setDefaultDestination(defaultExtractionDestination());
            dlg.setSelectionCount(m_model.info().fileCount, m_model.info().dirCount);
            if (dlg.exec() != QDialog::Accepted)
                return ProcessOutcome{Diagnostic::Cancelled, ExitCode::UserBreak, {}, true};
            ExtractOptions options = dlg.options(m_hasPassword, m_password);
            if (!confirmExtractionOverwrites(options))
                return ProcessOutcome{Diagnostic::Cancelled, ExitCode::UserBreak, {}, true};
            destination = options.destination;
            showInExplorer = dlg.showInExplorer();
            return m_service.extract(m_model.archivePath(), options);
        });
        if (extracted && showInExplorer)
            QDesktopServices::openUrl(QUrl::fromLocalFile(destination));
        break;
    }
    case CommandRegistry::CmdVirusScan: {
        // WinRAR llama al antivirus del sistema. Aqui se hace lo mismo: se
        // busca un analizador de linea de comandos y se le pasa lo extraido.
        if (m_model.archivePath().isEmpty()) {
            statusBar()->showMessage(tr("Abre primero un archivo."), 4000);
            break;
        }
        runOperation(tr("Análisis antivirus"), 1, [this](ProgressDialog *, bool *) {
            VirusScan scan;
            const QString scanner = scan.findScanner();
            if (scanner.isEmpty())
                return ProcessOutcome{Diagnostic::ToolNotFound, ExitCode::CommandLineError,
                                      tr("No hay ningún analizador de virus de línea de comandos "
                                         "instalado (clamdscan, clamscan o scanson)."),
                                      false};
            return scan.scanArchive(m_service, m_model.archivePath(), m_password, scanner);
        });
        break;
    }
    case CommandRegistry::CmdProtect: {
        if (m_model.archivePath().isEmpty()) {
            statusBar()->showMessage(tr("Abre primero un archivo."), 4000);
            break;
        }
        runOperation(tr("Proteger archivo"), 1, [this](ProgressDialog *, bool *) {
            return m_service.addRecoveryRecord(m_model.archivePath());
        });
        break;
    }
    case CommandRegistry::CmdRecover: {
        // WinRAR deja "Reparar" disponible sin archivo abierto; entonces hay
        // que elegir uno. Con uno abierto, se repara el que haya.
        QString path = m_model.archivePath();
        if (path.isEmpty()) {
            path = QFileDialog::getOpenFileName(
                this, tr("Reparar archivo"), QString(),
                tr("Archivos RAR (*.rar);;Todos los archivos (*)"));
            if (path.isEmpty())
                break;
        }
        if (ArchiveService::detectFormat(path) != ArchiveFormat::Rar) {
            ProcessOutcome bad;
            bad.diagnostic = Diagnostic::OpenError;
            bad.exitCode = ExitCode::CommandLineError;
            bad.message = tr("Solo los archivos RAR se pueden reparar.");
            reportError(bad);
            break;
        }
        runOperation(tr("Reparar"), 1, [this, path](ProgressDialog *, bool *) {
            return m_service.recoverArchive(path);
        });
        break;
    }
    case CommandRegistry::CmdEncryptNames: {
        if (m_model.archivePath().isEmpty()) {
            statusBar()->showMessage(tr("Abre primero un archivo."), 4000);
            break;
        }
        if (ArchiveService::detectFormat(m_model.archivePath()) == ArchiveFormat::Zip) {
            reportError(ProcessOutcome{Diagnostic::OpenError, ExitCode::CommandLineError,
                                       tr("Un ZIP no admite cifrar los nombres: el formato no "
                                          "tiene esa posibilidad."), false});
            break;
        }
        PasswordDialog pwd(this);
        pwd.setWindowTitle(tr("Cifrar los nombres del archivo"));
        if (pwd.exec() != QDialog::Accepted || pwd.password().isEmpty())
            break;
        // Cifrar los nombres obliga a rehacer el archivo entero, asi que se
        // avisa antes: el original se sustituye al final, no durante.
        if (QMessageBox::question(
                this, tr("Cifrar los nombres del archivo"),
                tr("RARLAB no puede poner cifrado a los nombres de un archivo que ya existe. "
                   "QtRAR lo deshará y lo volverá a crear cifrándolo entero, y solo sustituirá "
                   "el original cuando el nuevo esté listo.\n\n¿Seguir?"))
            != QMessageBox::Yes)
            break;
        const QString original = m_model.archivePath();
        runOperation(tr("Cifrando los nombres"), 1,
                     [this, original, &pwd](ProgressDialog *, bool *) {
                         return m_service.encryptFileNames(original, pwd.password(), QString(),
                                                           &m_stagedArchive);
                     });
        if (!m_stagedArchive.isEmpty()) {
            const QString staged = m_stagedArchive;
            m_stagedArchive.clear();
            if (QFile::exists(original) && !QFile::remove(original)) {
                QMessageBox::warning(this, tr("Error"),
                                     tr("No se pudo apartar el archivo original: %1").arg(original));
                QFile::remove(staged);
            } else if (!QFile::rename(staged, original)) {
                QMessageBox::warning(this, tr("Error"),
                                     tr("No se pudo poner el archivo nuevo en su sitio."));
            } else {
                openArchive(original);
            }
        }
        break;
    }
    case CommandRegistry::CmdSfx: {
        // El modulo autoextraible de WinRAR (`sfx*.exe`) es un binario de
        // Windows. RARLAB no lo publica para Linux, y meterse con wine para
        // esto no compensa: mejor decirlo claro que fingir que funciona.
        QMessageBox::information(
            this, tr("Crear archivo autoextraíble"),
            tr("El módulo autoextraíble que usa WinRAR es un ejecutable de Windows y RARLAB no "
               "publica una versión para Linux. Por eso QtRAR no puede crear archivos "
               "autoextraíbles.\n\nUn RAR normal se abre con doble clic y trae su propio "
               "descompresor, así que casi siempre es suficiente."));
        break;
    }
    default:
        statusBar()->showMessage(
            tr("La acción \"%1\" todavía no está implementada.").arg(m_commands.action(id)
                                                                          ? m_commands.action(id)->text()
                                                                          : QString()),
            4000);
        break;
    }
}

void MainWindow::onContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    const QStringList sel = selectedPaths();
    const bool hasSel = !sel.isEmpty();
    const bool hasArchive = m_model.hasArchive();

    for (Id id : {CmdExtractTo, CmdExtractSelected, CmdTest, CmdView, CmdDelete, CmdRename,
                  CmdAddToArchive, CmdInfo}) {
        if (QAction *a = m_commands.action(id)) {
            a->setEnabled(a->isEnabled() && (hasArchive || id == CommandRegistry::CmdAddToArchive));
            menu.addAction(a);
        }
    }
    menu.addSeparator();
    for (Id id : {CmdSelectAll, CmdDeselectAll, CmdInvertSelection}) {
        if (QAction *a = m_commands.action(id)) {
            a->setEnabled(hasArchive);
            menu.addAction(a);
        }
    }
    Q_UNUSED(hasSel)
    menu.exec(m_view->viewport()->mapToGlobal(pos));
}

void MainWindow::onLicenseStatusChanged(const LicenseStatus &status)
{
    m_licenseStatus = status;
    m_statusPanel->setLicenseText(LicenseProbe::stateText(status));
    m_statusPanel->setLicenseWarning(status.state == LicenseState::Evaluation);
    m_commands.setCanCreateArchives(status.canCreateArchives());
    m_statusPanel->setToolTip(status.detail);
}

bool MainWindow::runOperation(const QString &title, int steps,
                              std::function<ProcessOutcome(ProgressDialog *, bool *)> work)
{
    auto *dlg = new ProgressDialog(title, steps, this);
    m_progress = dlg;
    dlg->setAttribute(Qt::WA_DeleteOnClose, false);

    bool cancelledByUser = false;
    connect(dlg, &ProgressDialog::cancelled, this, [&cancelledByUser] {
        cancelledByUser = true;
    });
    dlg->show();
    QApplication::processEvents();

    bool serviceCancelled = false;
    const ProcessOutcome outcome = work(dlg, &serviceCancelled);

    cancelledByUser = cancelledByUser || dlg->wasCancelled();
    dlg->close();
    dlg->deleteLater();
    m_progress = nullptr;

    if (cancelledByUser || outcome.diagnostic == Diagnostic::Cancelled) {
        statusBar()->showMessage(tr("Operación cancelada."), 4000);
        return false;
    }
    if (outcome.success) {
        refreshView();
        return true;
    }
    reportError(outcome);
    return false;
}

void MainWindow::reportError(const ProcessOutcome &outcome)
{
    const QString title = tr("Error");
    switch (outcome.diagnostic) {
    case Diagnostic::ToolNotFound: {
        QMessageBox box(this);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(title);
        box.setText(tr("Falta la herramienta necesaria para esta operación."));
        box.setInformativeText(
            outcome.message.isEmpty()
                ? tr("No se ha encontrado el programa que hace este trabajo.")
                : outcome.message);
        QPushButton *zip = box.addButton(tr("Crear ZIP"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == zip)
            onCommand(CommandRegistry::CmdNewArchive);
        return;
    }

    case Diagnostic::LicenseRequired: {
        // Situacion prevista, no un fallo: se ofrece la alternativa libre.
        QMessageBox box(this);
        box.setIcon(QMessageBox::Information);
        box.setWindowTitle(title);
        box.setText(tr("El binario 'rar' no permite escribir en este archivo."));
        box.setInformativeText(
            tr("RARLAB permite crear y modificar durante los 40 días de evaluación; "
               "después hace falta una licencia de pago.\n\n"
               "Extraer, probar y abrir archivos nunca requiere licencia.\n\n"
               "¿Quieres crear un archivo ZIP en su lugar?"));
        QPushButton *zip = box.addButton(tr("Crear ZIP"), QMessageBox::AcceptRole);
        box.addButton(QMessageBox::Cancel);
        box.exec();
        if (box.clickedButton() == zip)
            onCommand(CommandRegistry::CmdNewArchive);
        return;
    }

    case Diagnostic::WrongPassword: {
        PasswordDialog dlg(this);
        dlg.setReason(tr("Contraseña incorrecta o ausente."));
        if (dlg.exec() != QDialog::Accepted)
            return;
        m_password = dlg.password();
        m_hasPassword = true;
        openArchive(m_model.archivePath());
        return;
    }

    case Diagnostic::VolumeMissing: {
        QString missing;
        if (!m_service.volumesPresent(m_model.archivePath(), &missing)) {
            QMessageBox::warning(this, title,
                                 tr("Falta un volumen de la serie.\n\nSe esperaba:\n%1")
                                     .arg(QDir::toNativeSeparators(missing)));
        } else {
            reportSuccess(tr("Todos los volúmenes están presentes."));
        }
        return;
    }

    default:
        break;
    }

    QString text = Diagnostics::describe(outcome);
    if (!outcome.message.isEmpty() && !text.contains(outcome.message))
        text += QStringLiteral("\n\n") + outcome.message;
    QMessageBox::warning(this, title, text);
}

void MainWindow::reportSuccess(const QString &message)
{
    statusBar()->showMessage(message, 4000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    settings.setValue(QStringLiteral("window/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("window/state"), saveState());
    settings.setValue(QStringLiteral("favorites/list"), m_favorites);
    QMainWindow::closeEvent(event);
}

// ---------------------------------------------------------------------------
// Volcado de la interfaz, para pruebas y depuracion
// ---------------------------------------------------------------------------

void MainWindow::dumpUi(QTextStream &out) const
{
    out << "TITULO: " << windowTitle() << '\n';

    out << "MENUS:";
    for (QAction *menuAction : menuBar()->actions())
        out << ' ' << menuAction->text();
    out << "\n";

    for (QAction *menuAction : menuBar()->actions()) {
        auto *menu = qobject_cast<QMenu *>(menuAction->menu());
        if (!menu)
            continue;
        out << "  " << menuAction->text() << ":";
        for (QAction *a : menu->actions()) {
            if (a->isSeparator()) {
                out << " | ";
                continue;
            }
            out << ' ' << (a->isEnabled() ? '+' : '-') << a->text();
        }
        out << '\n';
    }

    out << "BARRA:";
    for (QAction *a : m_toolBar->actions()) {
        if (a->isSeparator()) {
            out << " | ";
            continue;
        }
        out << ' ' << (a->isEnabled() ? '+' : '-') << a->text();
    }
    out << '\n';

    // Lo que de verdad se ve bajo el icono: el `iconText`. Si sigue el texto
    // del menu, la barra sale con "Eliminar..." y con los "&" de los atajos.
    out << "  etiquetas:";
    for (QAction *a : m_toolBar->actions()) {
        if (!a->isSeparator())
            out << " [" << a->iconText() << ']';
    }
    out << '\n';
    int ancho = 0;
    for (QAction *a : m_toolBar->actions()) {
        if (auto *b = qobject_cast<QToolButton *>(m_toolBar->widgetForAction(a)))
            ancho += b->sizeHint().width() + 2;
    }
    out << "  estilo=" << int(m_toolBar->toolButtonStyle())
        << " icono=" << m_toolBar->iconSize().width() << "x" << m_toolBar->iconSize().height()
        << " ancho=" << ancho << '/' << m_toolBar->width()
        << (ancho > m_toolBar->width() ? " DESBORDA" : " cabe")
        << '\n';

    out << "DIRECCION: " << m_addressBar->displayText() << '\n';
    out << "COMENTARIO_VISIBLE: " << (m_commentView->isVisible() ? 1 : 0);
    if (!m_commentView->toPlainText().isEmpty()) {
        out << " texto=" << QString::fromLatin1(
            m_commentView->toPlainText().toUtf8().toPercentEncoding());
    }
    out << '\n';
    if (m_fileSystemMode) {
        const QModelIndex root = m_fileSystemModel->index(m_fileSystemPath);
        out << "VISTA: sistema de archivos\n";
        out << "TABLA: " << m_fileSystemModel->rowCount(root) << " filas, 4 columnas\n";
        for (int row = 0; row < m_fileSystemModel->rowCount(root); ++row) {
            out << "  ";
            for (int column = 0; column < 4; ++column) {
                const QString text = m_fileSystemModel->data(
                    m_fileSystemModel->index(row, column, root)).toString();
                out << (column ? " | " : " ") << (text.isEmpty() ? "-" : text);
            }
            out << '\n';
        }
    } else {
        out << "VISTA: archivo\n";
        out << "TABLA: " << m_proxy.rowCount() << " filas, "
            << m_proxy.columnCount() << " columnas\n";
        out << "  ";
        for (int c = 0; c < m_proxy.columnCount(); ++c)
            out << '[' << m_model.headerText(c) << "] ";
        out << '\n';
        for (int r = 0; r < m_proxy.rowCount(); ++r) {
            out << "  ";
            for (int c = 0; c < m_proxy.columnCount(); ++c) {
                const QString text = m_proxy.data(m_proxy.index(r, c)).toString();
                out << (c ? " | " : " ") << (text.isEmpty() ? "-" : text);
            }
            out << '\n';
        }
    }
    out << "ESTADO: " << m_statusPanel->countsText() << '\n';
    out.flush();
}

} // namespace qtrar
