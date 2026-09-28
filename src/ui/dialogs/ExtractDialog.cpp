#include "ExtractDialog.h"

#include "ui/ThemeManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSettings>
#include <QShowEvent>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTreeView>
#include <QTimer>
#include <QVBoxLayout>

namespace qtrar {

ExtractDialog::ExtractDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Extraer archivos"));
    setWindowIcon(ThemeManager::icon(QStringLiteral("app")));
    setModal(true);
    setMinimumSize(700, 440);
    resize(760, 500);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(10, 8, 10, 8);
    root->setSpacing(7);

    m_tabs = new QTabWidget(this);
    m_tabs->setObjectName(QStringLiteral("extractTabs"));
    root->addWidget(m_tabs, 1);

    // --- General: destination, update/overwrite policy, directory tree -----
    auto *general = new QWidget(m_tabs);
    auto *generalLayout = new QVBoxLayout(general);
    generalLayout->setContentsMargins(9, 8, 9, 6);
    generalLayout->setSpacing(6);

    m_archiveLabel = new QLabel(general);
    m_archiveLabel->setObjectName(QStringLiteral("extractArchiveLabel"));
    m_archiveLabel->setTextFormat(Qt::PlainText);
    generalLayout->addWidget(m_archiveLabel);
    generalLayout->addWidget(new QLabel(
        tr("Carpeta de destino (si no existe, se creará)"), general));

    auto *destinationRow = new QHBoxLayout;
    destinationRow->setSpacing(4);
    m_destinationCombo = new QComboBox(general);
    m_destinationCombo->setObjectName(QStringLiteral("extractDestination"));
    m_destinationCombo->setEditable(true);
    m_destinationCombo->setInsertPolicy(QComboBox::InsertAtTop);
    m_destinationCombo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    m_destinationCombo->setMinimumContentsLength(34);
    auto *showButton = new QPushButton(tr("Mostrar"), general);
    auto *newFolderButton = new QPushButton(tr("Nueva carpeta"), general);
    destinationRow->addWidget(m_destinationCombo, 1);
    auto *destinationButtons = new QVBoxLayout;
    destinationButtons->setSpacing(3);
    destinationButtons->addWidget(showButton);
    destinationButtons->addWidget(newFolderButton);
    destinationRow->addLayout(destinationButtons);
    generalLayout->addLayout(destinationRow);

    auto *body = new QHBoxLayout;
    body->setSpacing(9);
    auto *leftPanel = new QWidget(general);
    auto *leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(6);

    auto *updateBox = new QGroupBox(tr("Modo de actualización"), leftPanel);
    auto *updateLayout = new QVBoxLayout(updateBox);
    updateLayout->setContentsMargins(9, 7, 7, 7);
    updateLayout->setSpacing(2);
    m_extractReplace = new QRadioButton(tr("Extraer y reemplazar"), updateBox);
    m_extractUpdate = new QRadioButton(tr("Extraer y actualizar"), updateBox);
    m_extractFreshen = new QRadioButton(tr("Solo actualizar"), updateBox);
    m_extractReplace->setObjectName(QStringLiteral("extractReplace"));
    m_extractUpdate->setObjectName(QStringLiteral("extractUpdate"));
    m_extractFreshen->setObjectName(QStringLiteral("extractFreshen"));
    updateLayout->addWidget(m_extractReplace);
    updateLayout->addWidget(m_extractUpdate);
    updateLayout->addWidget(m_extractFreshen);
    m_extractReplace->setChecked(true);
    leftLayout->addWidget(updateBox);

    auto *overwriteBox = new QGroupBox(tr("Modo sobrescribir"), leftPanel);
    auto *overwriteLayout = new QVBoxLayout(overwriteBox);
    overwriteLayout->setContentsMargins(9, 7, 7, 7);
    overwriteLayout->setSpacing(2);
    m_confirmExisting = new QRadioButton(tr("Confirmar sobrescritura"), overwriteBox);
    m_overwriteExisting = new QRadioButton(tr("Sobrescribir los archivos existentes"),
                                           overwriteBox);
    m_skipExisting = new QRadioButton(tr("Omitir los archivos existentes"), overwriteBox);
    m_renameExisting = new QRadioButton(tr("Renombrar automáticamente"), overwriteBox);
    m_confirmExisting->setObjectName(QStringLiteral("extractConfirmOverwrite"));
    m_overwriteExisting->setObjectName(QStringLiteral("extractOverwrite"));
    m_skipExisting->setObjectName(QStringLiteral("extractSkip"));
    m_renameExisting->setObjectName(QStringLiteral("extractRename"));
    overwriteLayout->addWidget(m_confirmExisting);
    overwriteLayout->addWidget(m_overwriteExisting);
    overwriteLayout->addWidget(m_skipExisting);
    overwriteLayout->addWidget(m_renameExisting);
    m_confirmExisting->setChecked(true);
    leftLayout->addWidget(overwriteBox);

    auto *miscBox = new QGroupBox(tr("Varios"), leftPanel);
    auto *miscLayout = new QVBoxLayout(miscBox);
    miscLayout->setContentsMargins(9, 7, 7, 7);
    miscLayout->setSpacing(2);
    m_keepBroken = new QCheckBox(tr("Conservar archivos dañados"), miscBox);
    m_showInExplorer = new QCheckBox(tr("Mostrar archivos en el explorador al terminar"), miscBox);
    m_keepBroken->setObjectName(QStringLiteral("extractKeepBroken"));
    m_showInExplorer->setObjectName(QStringLiteral("extractShowInExplorer"));
    miscLayout->addWidget(m_keepBroken);
    miscLayout->addWidget(m_showInExplorer);
    leftLayout->addWidget(miscBox);
    leftLayout->addStretch(1);

    m_folderModel = new QFileSystemModel(this);
    m_folderModel->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Drives | QDir::Hidden);
    m_folderModel->setRootPath(QDir::rootPath());
    connect(m_folderModel, &QFileSystemModel::directoryLoaded, this,
            [this](const QString &loadedPath) {
                const QString target = QDir::cleanPath(
                    QDir(m_destinationCombo->currentText()).absolutePath());
                const QString loaded = QDir::cleanPath(loadedPath);
                const bool underLoaded = loaded == QDir::rootPath()
                    ? target.startsWith(loaded)
                    : target == loaded || target.startsWith(loaded + QDir::separator());
                if (underLoaded)
                    showDestinationInTree();
            });
    m_folderTree = new QTreeView(general);
    m_folderTree->setObjectName(QStringLiteral("extractFolderTree"));
    m_folderTree->setModel(m_folderModel);
    m_folderTree->setRootIndex(m_folderModel->index(QDir::rootPath()));
    m_folderTree->setHeaderHidden(true);
    m_folderTree->setMinimumWidth(235);
    m_folderTree->setColumnHidden(1, true);
    m_folderTree->setColumnHidden(2, true);
    m_folderTree->setColumnHidden(3, true);
    m_folderTree->setExpandsOnDoubleClick(true);
    m_folderTree->header()->setStretchLastSection(false);
    m_folderTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);

    body->addWidget(leftPanel, 1);
    body->addWidget(m_folderTree, 1);
    generalLayout->addLayout(body, 1);

    auto *saveRow = new QHBoxLayout;
    auto *saveButton = new QPushButton(tr("Guardar opciones"), general);
    saveButton->setObjectName(QStringLiteral("extractSaveOptions"));
    saveRow->addWidget(saveButton, 1);
    saveRow->addStretch(1);
    generalLayout->addLayout(saveRow);
    m_tabs->addTab(general, tr("General"));

    // --- Advanced: destination layout, paths and password ------------------
    auto *advanced = new QWidget(m_tabs);
    auto *advancedLayout = new QVBoxLayout(advanced);
    advancedLayout->setContentsMargins(12, 12, 12, 12);

    auto *destinationMode = new QGroupBox(tr("Carpeta de extracción"), advanced);
    auto *destinationModeLayout = new QVBoxLayout(destinationMode);
    m_extractToFolder = new QRadioButton(tr("Extraer en una subcarpeta del archivo"),
                                         destinationMode);
    m_extractFilesTo = new QRadioButton(tr("Extraer directamente en la carpeta de destino"),
                                        destinationMode);
    m_extractToFolder->setObjectName(QStringLiteral("extractIntoArchiveFolder"));
    m_extractFilesTo->setObjectName(QStringLiteral("extractDirectly"));
    m_extractToFolder->setChecked(true);
    destinationModeLayout->addWidget(m_extractToFolder);
    destinationModeLayout->addWidget(m_extractFilesTo);
    advancedLayout->addWidget(destinationMode);

    auto *pathsBox = new QGroupBox(tr("Rutas de carpeta"), advanced);
    auto *pathsLayout = new QVBoxLayout(pathsBox);
    m_dontExtractPaths = new QCheckBox(tr("No extraer las rutas de las carpetas"), pathsBox);
    m_dontExtractPaths->setObjectName(QStringLiteral("extractWithoutPaths"));
    pathsLayout->addWidget(m_dontExtractPaths);
    advancedLayout->addWidget(pathsBox);

    auto *passwordBox = new QGroupBox(tr("Contraseña"), advanced);
    auto *passwordLayout = new QFormLayout(passwordBox);
    m_passwordEdit = new QLineEdit(passwordBox);
    m_passwordEdit->setObjectName(QStringLiteral("extractPassword"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_showPassword = new QCheckBox(tr("Mostrar contraseña"), passwordBox);
    connect(m_showPassword, &QCheckBox::toggled, this, [this](bool show) {
        m_passwordEdit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });
    passwordLayout->addRow(tr("Contraseña:"), m_passwordEdit);
    passwordLayout->addRow(QString(), m_showPassword);
    advancedLayout->addWidget(passwordBox);
    advancedLayout->addStretch(1);
    m_tabs->addTab(advanced, tr("Avanzado"));

    // --- Options: persistent preferences -----------------------------------
    auto *optionsPage = new QWidget(m_tabs);
    auto *optionsLayout = new QVBoxLayout(optionsPage);
    optionsLayout->setContentsMargins(12, 12, 12, 12);
    m_rememberOptions = new QCheckBox(tr("Recordar estas opciones para futuras extracciones"),
                                      optionsPage);
    m_rememberOptions->setObjectName(QStringLiteral("extractRememberOptions"));
    optionsLayout->addWidget(m_rememberOptions);
    auto *hint = new QLabel(tr("La carpeta de destino y las opciones de extracción se pueden "
                               "guardar desde la pestaña General."), optionsPage);
    hint->setWordWrap(true);
    optionsLayout->addWidget(hint);
    optionsLayout->addStretch(1);
    m_tabs->addTab(optionsPage, tr("Opciones"));

    auto *buttons = new QDialogButtonBox(this);
    auto *helpButton = buttons->addButton(tr("Ayuda"), QDialogButtonBox::HelpRole);
    buttons->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    m_extractButton = buttons->addButton(tr("Extraer"), QDialogButtonBox::AcceptRole);
    m_extractButton->setObjectName(QStringLiteral("extractAcceptButton"));
    m_extractButton->setDefault(true);
    root->addWidget(buttons);

    connect(showButton, &QPushButton::clicked, this, &ExtractDialog::showDestinationInTree);
    connect(newFolderButton, &QPushButton::clicked, this, &ExtractDialog::createDestinationFolder);
    connect(saveButton, &QPushButton::clicked, this, &ExtractDialog::saveOptions);
    connect(m_folderTree, &QTreeView::clicked, this, [this](const QModelIndex &index) {
        const QString path = m_folderModel->filePath(index);
        if (QFileInfo(path).isDir())
            m_destinationCombo->setCurrentText(QDir::toNativeSeparators(path));
    });
    connect(m_destinationCombo->lineEdit(), &QLineEdit::textChanged,
            this, &ExtractDialog::updateOkState);
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        if (m_rememberOptions->isChecked())
            saveOptions();
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(helpButton, &QPushButton::clicked, this, [this] {
        QMessageBox::information(this, tr("Extraer archivos"),
                                 tr("Elige la carpeta de destino y ajusta las opciones de "
                                    "extracción en las pestañas General y Avanzado."));
    });

    QSettings settings;
    const QString savedDestination = settings.value(QStringLiteral("extract/destination"),
        QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
    m_destinationCombo->addItems(settings.value(QStringLiteral("extract/destinations")).toStringList());
    m_destinationCombo->setCurrentText(savedDestination);
    m_rememberOptions->setChecked(settings.value(QStringLiteral("extract/remember"), false).toBool());
    m_keepBroken->setChecked(settings.value(QStringLiteral("extract/keepBroken"), false).toBool());
    m_dontExtractPaths->setChecked(settings.value(QStringLiteral("extract/preservePaths"), true)
                                       .toBool() == false);
    m_showInExplorer->setChecked(settings.value(QStringLiteral("extract/showInExplorer"), false)
                                     .toBool());
    const int defaultOverwrite = settings.value(QStringLiteral("files/confirmOverwrite"), true)
                                     .toBool() ? 0 : 2;
    const int overwriteMode = settings.value(QStringLiteral("extract/existingFiles"),
                                              defaultOverwrite).toInt();
    (overwriteMode == 1 ? m_overwriteExisting : overwriteMode == 2 ? m_skipExisting
        : overwriteMode == 3 ? m_renameExisting : m_confirmExisting)->setChecked(true);
    const int updateMode = settings.value(QStringLiteral("extract/updateMode"), 0).toInt();
    (updateMode == 1 ? m_extractUpdate : updateMode == 2 ? m_extractFreshen
                                                         : m_extractReplace)->setChecked(true);
    const int destinationModeIndex = settings.value(QStringLiteral("extract/destinationMode"), 1)
                                         .toInt();
    (destinationModeIndex == 1 ? m_extractFilesTo : m_extractToFolder)->setChecked(true);
    showDestinationInTree();
    updateOkState();
}

void ExtractDialog::setArchiveName(const QString &archiveName)
{
    m_archiveLabel->setText(tr("Archivo: %1").arg(archiveName));
    m_archiveBaseName = QFileInfo(archiveName).completeBaseName();
    setWindowTitle(tr("Extraer en %1").arg(QFileInfo(archiveName).fileName()));
    const bool isZip = QFileInfo(archiveName).suffix().compare(QLatin1String("zip"),
                                                              Qt::CaseInsensitive) == 0;
    m_extractUpdate->setEnabled(!isZip);
    m_extractFreshen->setEnabled(!isZip);
    if (isZip && (m_extractUpdate->isChecked() || m_extractFreshen->isChecked()))
        m_extractReplace->setChecked(true);
    if (m_extractToFolder->isChecked())
        m_extractToFolder->setText(tr("Extraer en %1\\").arg(m_archiveBaseName));
}

void ExtractDialog::setSelectedPaths(const QStringList &paths)
{
    m_selected = paths;
}

void ExtractDialog::setDefaultDestination(const QString &path)
{
    if (!path.isEmpty())
        m_destinationCombo->setCurrentText(QDir::toNativeSeparators(path));
    showDestinationInTree();
    updateOkState();
}

void ExtractDialog::setSelectionCount(int files, int folders)
{
    m_files = files;
    m_folders = folders;
    if (files == 0 && folders == 0) {
        m_extractToFolder->setText(tr("Extraer en %1\\").arg(m_archiveBaseName));
    } else {
        m_extractToFolder->setText(tr("Extraer los archivos seleccionados (%1 archivos, %2 carpetas)")
                                       .arg(files).arg(folders));
    }
    updateOkState();
}

QString ExtractDialog::destination() const
{
    const QString base = m_destinationCombo->currentText().trimmed();
    if (base.isEmpty())
        return {};
    if (m_extractToFolder->isChecked() && !m_archiveBaseName.isEmpty())
        return QDir(base).absoluteFilePath(m_archiveBaseName);
    return QDir(base).absolutePath();
}

void ExtractDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    // QFileSystemModel carga directorios de forma asincrona. Al mostrarse el
    // dialogo el destino inicial ya puede estar disponible: expandir y llevar
    // el arbol a esa carpeta hace que la seleccion coincida con el campo.
    QTimer::singleShot(0, this, &ExtractDialog::showDestinationInTree);
}

bool ExtractDialog::hasPassword() const
{
    return !m_passwordEdit->text().isEmpty();
}

QString ExtractDialog::password() const
{
    return m_passwordEdit->text();
}

bool ExtractDialog::showInExplorer() const
{
    return m_showInExplorer->isChecked();
}

ExtractOptions ExtractDialog::options(bool hasPassword, const QString &password) const
{
    ExtractOptions o;
    o.destination = destination();
    o.items = selectedPaths();
    o.keepBrokenFiles = m_keepBroken->isChecked();
    o.preservePaths = !m_dontExtractPaths->isChecked();
    o.hasPassword = hasPassword;
    o.password = password;
    o.existingFiles = m_confirmExisting->isChecked() ? ExistingFilesMode::Confirm
        : m_skipExisting->isChecked() ? ExistingFilesMode::Skip
        : m_renameExisting->isChecked() ? ExistingFilesMode::Rename
                                        : ExistingFilesMode::Overwrite;
    o.updateMode = m_extractUpdate->isChecked() ? ExtractUpdateMode::Update
        : m_extractFreshen->isChecked() ? ExtractUpdateMode::Freshen
                                        : ExtractUpdateMode::Replace;
    return o;
}

void ExtractDialog::showDestinationInTree()
{
    const QString path = QDir::cleanPath(QDir(m_destinationCombo->currentText()).absolutePath());
    if (!QFileInfo(path).isDir())
        return;
    QModelIndex index = m_folderModel->index(path);
    if (!index.isValid())
        return;
    QModelIndex ancestor = index.parent();
    while (ancestor.isValid()) {
        m_folderTree->expand(ancestor);
        ancestor = ancestor.parent();
    }
    m_folderTree->setCurrentIndex(index);
    m_folderTree->scrollTo(index, QAbstractItemView::PositionAtCenter);
    // Qt puede resolver el QModelIndex mientras aun esta reconstruyendo las
    // filas asincronas del arbol. Reintentar tras el siguiente layout mantiene
    // el destino visible y seleccionado, tambien en el primer `exec()`.
    QTimer::singleShot(60, this, [this, path] {
        const QModelIndex target = m_folderModel->index(path);
        if (!target.isValid())
            return;
        for (QModelIndex ancestor = target.parent(); ancestor.isValid();
             ancestor = ancestor.parent()) {
            m_folderTree->expand(ancestor);
        }
        m_folderTree->setCurrentIndex(target);
        m_folderTree->scrollTo(target, QAbstractItemView::PositionAtCenter);
    });
}

void ExtractDialog::createDestinationFolder()
{
    const QString name = QInputDialog::getText(this, tr("Nueva carpeta"),
                                                tr("Nombre de la carpeta:"));
    if (name.trimmed().isEmpty())
        return;
    QDir parent(m_destinationCombo->currentText());
    if (!parent.exists() || !parent.mkdir(name.trimmed())) {
        QMessageBox::warning(this, tr("Nueva carpeta"),
                             tr("No se pudo crear la carpeta en el destino seleccionado."));
        return;
    }
    m_destinationCombo->setCurrentText(parent.absoluteFilePath(name.trimmed()));
    showDestinationInTree();
}

void ExtractDialog::saveOptions()
{
    QSettings settings;
    QStringList destinations = settings.value(QStringLiteral("extract/destinations")).toStringList();
    const QString destination = m_destinationCombo->currentText().trimmed();
    if (!destination.isEmpty()) {
        destinations.removeAll(destination);
        destinations.prepend(destination);
        while (destinations.size() > 10)
            destinations.removeLast();
        m_destinationCombo->insertItem(0, destination);
        m_destinationCombo->setCurrentIndex(0);
    }
    settings.setValue(QStringLiteral("extract/destination"), destination);
    settings.setValue(QStringLiteral("extract/destinations"), destinations);
    settings.setValue(QStringLiteral("extract/remember"), m_rememberOptions->isChecked());
    settings.setValue(QStringLiteral("extract/keepBroken"), m_keepBroken->isChecked());
    settings.setValue(QStringLiteral("extract/preservePaths"), !m_dontExtractPaths->isChecked());
    settings.setValue(QStringLiteral("extract/showInExplorer"), m_showInExplorer->isChecked());
    settings.setValue(QStringLiteral("extract/existingFiles"), m_confirmExisting->isChecked() ? 0
        : m_overwriteExisting->isChecked() ? 1 : m_skipExisting->isChecked() ? 2 : 3);
    settings.setValue(QStringLiteral("files/confirmOverwrite"), m_confirmExisting->isChecked());
    settings.setValue(QStringLiteral("extract/updateMode"), m_extractUpdate->isChecked() ? 1
        : m_extractFreshen->isChecked() ? 2 : 0);
    settings.setValue(QStringLiteral("extract/destinationMode"),
                      m_extractFilesTo->isChecked() ? 1 : 0);
}

void ExtractDialog::updateOkState()
{
    const bool ok = !m_destinationCombo->currentText().trimmed().isEmpty();
    if (m_extractButton)
        m_extractButton->setEnabled(ok);
}

} // namespace qtrar
