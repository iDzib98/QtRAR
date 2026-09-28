// Ventana principal: replica la estructura de WinRAR.
//
//   Menus:  File  Commands  Tools  Favorites  Options  Help
//   Toolbar WinRAR agrupada y con botones segun el estado del archivo
//   Barra de direcciones con historial y boton de subir
//   Navegador del sistema de archivos o vista interna del archivo
//   Panel inferior con recuento y espacio libre
#pragma once

#include "core/ArchiveService.h"
#include "core/LicenseProbe.h"
#include "model/ArchiveTreeModel.h"
#include "ui/CommandRegistry.h"

#include <QMainWindow>
#include <QTextStream>
#include "ui/FindProxyModel.h"
#include <QStringList>
#include <memory>
#include <vector>

class QLabel;
class QMenu;
class QFileSystemModel;
class QSortFilterProxyModel;
class QStackedWidget;
class QSplitter;
class QTextBrowser;
class QToolBar;
class QTreeView;
class QTemporaryDir;

namespace qtrar {

class AddressBar;
class StatusPanel;
class ProgressDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// Abre un archivo. `asCommandLine` marca que viene de la linea de ordenes.
    bool openArchive(const QString &path, bool asCommandLine = false);

    /// "Acerca de": versiones de QtRAR y de las herramientas externas.
    void showAboutDialog();

    /// Ficha tecnica del archivo abierto (formato, tamano, cifrado...).
    void showArchiveInfo();

    /// Editor del comentario que el archivo lleva en su cabecera.
    void showCommentEditor();

    /// Ayuda de un vistazo, con los atajos y de quien depende cada formato.
    void showHelp();
    void showLicenseDialog();
    bool configureRarBinary();

    /// Flujo de "Nuevo archivo" / "Añadir al archivo".
    void createArchive(const QString &suggestedPath, const QStringList &initialFiles = {});
    /// Lanza una acción registrada desde una petición contextual al arrancar.
    void dispatchCommand(CommandRegistry::Id id) { onCommand(id); }

    /// Navega una carpeta local; la ruta se normaliza y se muestra en la vista.
    bool browseDirectory(const QString &path);
    bool browsingFileSystem() const { return m_fileSystemMode; }
    QString fileSystemPath() const { return m_fileSystemPath; }

    /// Interfaz de una sola instancia: recibe peticiones de otro proceso.
    void openSecondaryInstanceRequest(const QString &path);

    ArchiveService *service() { return &m_service; }

    /// Vuelca menus, barra y contenido a texto: sirve para comprobar el
    /// esqueleto de la ventana sin depender de una captura de pantalla.
    void dumpUi(QTextStream &out) const;

    /// Entra en una ruta interna del archivo (pruebas y "Ir a...").
    void goTo(const QString &internalPath);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void onCommand(qtrar::CommandRegistry::Id id);
    void onActivated(const QModelIndex &index);
    void onSelectionChanged();
    void onContextMenu(const QPoint &pos);
    void onAddressPathActivated(const QString &path);
    void onLicenseStatusChanged(const qtrar::LicenseStatus &status);

private:
    void buildMenus();
    void buildToolBar();
    void rebuildToolBarButtons();
    void buildCentralWidget();
    void buildStatusBar();

    void setupDropTarget();
    void refreshView();
    void configureArchiveView();
    void configureFileSystemView();
    void updateActionStates();
    void updateWindowTitle();
    void updateCounts();
    void updateCommentView(const QString &comment);
    QString defaultExtractionDestination() const;

    QStringList selectedPaths() const;
    bool confirmExtractionOverwrites(ExtractOptions &options);

    /// Ejecuta una operacion larga en segundo plano, mostrando el progreso.
    /// `steps` es el numero de miembros que se van a procesar.
    bool runOperation(const QString &title, int steps,
                      std::function<ProcessOutcome(ProgressDialog *, bool *)> work);

    /// Si la operacion necesita clave, la pide y reintenta una vez.
    template <typename Fn>
    bool withPassword(Fn &&fn);

    void reportError(const ProcessOutcome &outcome);
    void reportSuccess(const QString &message);

    ArchiveService m_service;
    LicenseProbe m_licenseProbe;
    LicenseStatus m_licenseStatus;
    CommandRegistry m_commands;
    ArchiveTreeModel m_model;
    FindProxyModel m_proxy;
    QFileSystemModel *m_fileSystemModel = nullptr;
    AddressBar *m_addressBar = nullptr;
    StatusPanel *m_statusPanel = nullptr;
    QTreeView *m_view = nullptr;
    QSplitter *m_contentSplitter = nullptr;
    QTextBrowser *m_commentView = nullptr;
    QToolBar *m_toolBar = nullptr;
    QStackedWidget *m_stack = nullptr;
    QMenu *m_favoritesMenu = nullptr;
    QStringList m_favorites;
    QString m_password;
    /// Los ficheros que se abren con la aplicacion asociada deben sobrevivir
    /// al retorno de QDesktopServices::openUrl(); se limpian al cerrar QtRAR.
    std::vector<std::unique_ptr<QTemporaryDir>> m_viewTemps;

    /// Archivo recien reempaquetado esperando a que se coloque en el sitio. Vive
    /// aqui entre `encryptFileNames()` y el intercambio, porque el temporal de
    /// aquel se borra al terminar.
    QString m_stagedArchive;
    bool m_hasPassword = false;
    /// Estado de "hay un archivo abierto". Decide que botones se ven en el
    /// bloque derecho de la barra: ver `MainWindow::rebuildToolBarButtons()`.
    bool m_archiveOpen = false;
    bool m_fileSystemMode = true;
    QString m_fileSystemPath;
    QString m_extractToFolderBase;
    ProgressDialog *m_progress = nullptr;
    QAction *m_quitAction = nullptr;
};

} // namespace qtrar
