#include "OptionsDialog.h"

#include "ui/ThemeManager.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace qtrar {

OptionsDialog::OptionsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Opciones"));

    auto *layout = new QVBoxLayout(this);
    auto *tabs = new QTabWidget(this);

    // --- General -------------------------------------------------------------
    auto *general = new QWidget(tabs);
    auto *generalForm = new QFormLayout(general);

    m_languageCombo = new QComboBox(general);
    // "auto" no aparece: se elige el idioma del sistema, que es lo que hace
    // falta y no tiene equivalente en el desplegable.
    m_languageCombo->addItem(tr("Español"), QStringLiteral("es"));
    m_languageCombo->addItem(tr("English"), QStringLiteral("en"));
    generalForm->addRow(tr("Idioma:"), m_languageCombo);

    m_singleInstanceCheck = new QCheckBox(tr("Una sola instancia de QtRAR"), general);
    generalForm->addRow(QString(), m_singleInstanceCheck);

    auto *pathRow = new QHBoxLayout;
    m_extractPathEdit = new QLineEdit(general);
    m_extractPathEdit->setObjectName(QStringLiteral("optionsExtractPathEdit"));
    auto *browse = new QPushButton(tr("Examinar..."), general);
    pathRow->addWidget(m_extractPathEdit, 1);
    pathRow->addWidget(browse);
    generalForm->addRow(tr("Carpeta de extracción:"), pathRow);

    connect(browse, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Carpeta de extracción"), m_extractPathEdit->text());
        if (!dir.isEmpty())
            m_extractPathEdit->setText(dir);
    });

    tabs->addTab(general, tr("General"));

    // --- Apariencia ----------------------------------------------------------
    auto *appearance = new QWidget(tabs);
    auto *appearanceForm = new QFormLayout(appearance);

    m_themeCombo = new QComboBox(appearance);
    m_themeCombo->addItem(tr("Clásico de WinRAR"), 0);
    m_themeCombo->addItem(tr("Del sistema"), 1);
    appearanceForm->addRow(tr("Tema:"), m_themeCombo);

    m_iconsCombo = new QComboBox(appearance);
    m_iconsCombo->addItem(tr("Los del programa"), 0);
    m_iconsCombo->addItem(tr("Los del sistema"), 1);
    appearanceForm->addRow(tr("Iconos:"), m_iconsCombo);

    tabs->addTab(appearance, tr("Apariencia"));

    // --- Extracción y creación -----------------------------------------------
    auto *files = new QWidget(tabs);
    auto *filesForm = new QFormLayout(files);

    m_confirmOverwriteCheck =
        new QCheckBox(tr("Preguntar antes de sobrescribir ficheros"), files);
    filesForm->addRow(QString(), m_confirmOverwriteCheck);

    m_createVolumesCheck = new QCheckBox(tr("Crear archivos por volumenes"), files);
    filesForm->addRow(QString(), m_createVolumesCheck);

    m_createLevelSpin = new QSpinBox(files);
    m_createLevelSpin->setRange(0, 5);
    m_createLevelSpin->setSuffix(tr(" (0 = sin compresión)"));
    filesForm->addRow(tr("Compresión por defecto:"), m_createLevelSpin);

    tabs->addTab(files, tr("Archivos"));

    // --- Ventana -------------------------------------------------------------
    auto *window = new QWidget(tabs);
    auto *windowForm = new QFormLayout(window);
    m_keepGeometryCheck = new QCheckBox(tr("Recordar posición y tamaño de la ventana"), window);
    windowForm->addRow(QString(), m_keepGeometryCheck);
    tabs->addTab(window, tr("Ventana"));

    layout->addWidget(tabs);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Aceptar"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    load();
}

void OptionsDialog::load()
{
    QSettings settings;
    const QString language = settings.value(QStringLiteral("ui/language"),
                                            QStringLiteral("es")).toString();
    const int index = m_languageCombo->findData(language);
    m_languageCombo->setCurrentIndex(index >= 0 ? index : 0);

    m_themeCombo->setCurrentIndex(settings.value(QStringLiteral("ui/theme"), 0).toInt());
    m_iconsCombo->setCurrentIndex(settings.value(QStringLiteral("ui/icons"), 0).toInt());
    m_singleInstanceCheck->setChecked(
        settings.value(QStringLiteral("app/singleInstance"), true).toBool());
    m_extractPathEdit->setText(
        settings.value(QStringLiteral("paths/extractTo")).toString());
    m_confirmOverwriteCheck->setChecked(
        settings.value(QStringLiteral("files/confirmOverwrite"), true).toBool());
    m_createVolumesCheck->setChecked(
        settings.value(QStringLiteral("files/createVolumes"), false).toBool());
    m_createLevelSpin->setValue(settings.value(QStringLiteral("files/compressionLevel"), 3).toInt());
    m_keepGeometryCheck->setChecked(
        settings.value(QStringLiteral("window/rememberGeometry"), true).toBool());

    m_originalLanguage = language;
}

bool OptionsDialog::apply()
{
    QSettings settings;

    const QString language = m_languageCombo->currentData().toString();
    const int theme = m_themeCombo->currentIndex();
    const int icons = m_iconsCombo->currentIndex();

    settings.setValue(QStringLiteral("ui/language"), language);
    settings.setValue(QStringLiteral("ui/theme"), theme);
    settings.setValue(QStringLiteral("ui/icons"), icons);
    settings.setValue(QStringLiteral("app/singleInstance"),
                      m_singleInstanceCheck->isChecked());
    settings.setValue(QStringLiteral("paths/extractTo"), m_extractPathEdit->text());
    settings.setValue(QStringLiteral("files/confirmOverwrite"),
                      m_confirmOverwriteCheck->isChecked());
    settings.setValue(QStringLiteral("files/createVolumes"),
                      m_createVolumesCheck->isChecked());
    settings.setValue(QStringLiteral("files/compressionLevel"), m_createLevelSpin->value());
    settings.setValue(QStringLiteral("window/rememberGeometry"),
                      m_keepGeometryCheck->isChecked());

    // El tema y los iconos se aplican ya: se nota el cambio sin reiniciar.
    ThemeManager::setIconSource(icons == 1 ? ThemeManager::IconSet::System
                                           : ThemeManager::IconSet::Original);
    ThemeManager::setTheme(theme == 1 ? ThemeManager::Theme::System
                                      : ThemeManager::Theme::Original);

    // El resto se nota al momento: el tema y los iconos ya se han aplicado, y
    // las demas preferencias se leen cuando hacen falta. Solo el idioma obliga a
    // reiniciar, porque las traducciones se cargan al arrancar.
    return language != m_originalLanguage;
}

} // namespace qtrar
