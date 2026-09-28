// Dialogo de extraccion, calcado en estructura al de WinRAR.
//
// Traduccion de cada opcion a los switches que verificamos en unrar 7.23:
//   "Extraer en"          destino como carpeta, el nombre del archivo se anade
//   "Extraer archivos en"  destino tal cual, sin crear la subcarpeta
//   "No extraer las rutas" -ep
//   "Conservar archivos dañados" -kb
//   "Sobrescribir"         -o+ / -o-
#pragma once

#include "core/ArchiveService.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QFileSystemModel;
class QLabel;
class QRadioButton;
class QLineEdit;
class QPushButton;
class QTreeView;
class QTabWidget;
class QShowEvent;

namespace qtrar {

class ExtractDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExtractDialog(QWidget *parent = nullptr);

    void setArchiveName(const QString &archiveName);
    void setDefaultDestination(const QString &path);

    ExtractOptions options(bool hasPassword, const QString &password) const;
    QString destination() const;
    bool hasPassword() const;
    QString password() const;

    /// Numero de entradas seleccionadas; 0 significa "todo el archivo".
    void setSelectionCount(int files, int folders);

    /// Rutas internas a extraer. Si esta vacia se extrae todo el archivo.
    /// WinRAR lo hace con una lista @listfiles; aqui se pasan como argumentos.
    void setSelectedPaths(const QStringList &paths);
    QStringList selectedPaths() const { return m_selected; }
    bool showInExplorer() const;

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void showDestinationInTree();
    void createDestinationFolder();
    void saveOptions();
    void updateOkState();

private:
    QLabel *m_archiveLabel;
    QComboBox *m_destinationCombo;
    QTreeView *m_folderTree;
    QFileSystemModel *m_folderModel;
    QTabWidget *m_tabs;
    QRadioButton *m_extractToFolder;  ///< "Extraer en <nombre>.rar\"
    QRadioButton *m_extractFilesTo;   ///< "Extraer archivos en"
    QRadioButton *m_extractReplace;
    QRadioButton *m_extractUpdate;
    QRadioButton *m_extractFreshen;
    QRadioButton *m_confirmExisting;
    QRadioButton *m_overwriteExisting;
    QRadioButton *m_skipExisting;
    QRadioButton *m_renameExisting;
    QCheckBox *m_keepBroken;
    QCheckBox *m_dontExtractPaths;
    QCheckBox *m_showInExplorer;
    QCheckBox *m_rememberOptions;
    QLineEdit *m_passwordEdit;
    QCheckBox *m_showPassword;
    QPushButton *m_extractButton = nullptr;
    int m_files = 0;
    int m_folders = 0;
    QString m_archiveBaseName;  ///< Sin extension, para la subcarpeta destino.
    QStringList m_selected;      ///< Rutas internas seleccionadas.
};

} // namespace qtrar
