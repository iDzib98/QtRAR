#include "CreateDialog.h"

#include "core/BinaryLocator.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

namespace qtrar {

namespace {

/// Traduce el nivel de compresion de RAR a como lo llama WinRAR. Se ofrece el
/// rango habitual -0..-5 en vez del 0..9 completo, que es lo que hace el
/// asistente de WinRAR y lo que la gente espera ver.
/// `QObject::tr` pondria estos textos en el contexto "QObject", que no es el
/// del dialogo y por tanto no se traducirian. Aqui se indica el bueno.
QString T(const char *text)
{
    return QCoreApplication::translate("qtrar::CreateDialog", text);
}

QString levelName(int level)
{
    switch (level) {
    case 0: return T("Sin compresión (rápido)");
    case 1: return T("Rápido");
    case 2: return T("Rápido (buena)");
    case 3: return T("Normal");
    case 4: return T("Buena");
    case 5: return T("Máxima");
    default: return T("Normal");
    }
}

QStringList splitFiles(const QString &text)
{
    // Se separan por ";" porque en un nombre de fichero puede haber comas.
    QStringList out;
    const QStringList parts = text.split(QLatin1Char(';'), Qt::SkipEmptyParts);
    for (const QString &p : parts) {
        const QString trimmed = p.trimmed();
        if (!trimmed.isEmpty())
            out.append(trimmed);
    }
    return out;
}

} // namespace

CreateDialog::CreateDialog(const QString &archivePath, Mode mode, QWidget *parent)
    : QDialog(parent), m_mode(mode)
{
    setWindowTitle(mode == Mode::New ? tr("Crear archivo nuevo")
                                     : tr("Añadir al archivo"));

    auto *layout = new QVBoxLayout(this);

    auto *label = new QLabel(
        mode == Mode::New
            ? tr("Escribe el nombre del archivo y los ficheros que quieres meter dentro.")
            : tr("Escribe o elige el archivo y lo que quieres añadir dentro."),
        this);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto *form = new QFormLayout;

    // --- Nombre del archivo --------------------------------------------------
    auto *pathRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit(archivePath, this);
    m_pathEdit->setObjectName(QStringLiteral("createPathEdit"));
    auto *pathBrowse = new QPushButton(tr("Examinar..."), this);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(pathBrowse);
    form->addRow(tr("Nombre del archivo:"), pathRow);

    connect(pathBrowse, &QPushButton::clicked, this, [this] {
        const QString start = m_pathEdit->text().isEmpty() ? QString()
                                                           : m_pathEdit->text();
        const QString file =
            QFileDialog::getSaveFileName(this, tr("Crear archivo"), start);
        if (!file.isEmpty())
            m_pathEdit->setText(file);
    });

    // --- Ficheros a incluir --------------------------------------------------
    auto *filesRow = new QHBoxLayout;
    m_filesEdit = new QLineEdit(this);
    m_filesEdit->setObjectName(QStringLiteral("createFilesEdit"));
    m_filesEdit->setPlaceholderText(tr("uno; dos; carpeta"));
    auto *filesBrowse = new QPushButton(tr("Añadir..."), this);
    auto *dirBrowse = new QPushButton(tr("Añadir carpeta..."), this);
    filesRow->addWidget(m_filesEdit, 1);
    filesRow->addWidget(filesBrowse);
    filesRow->addWidget(dirBrowse);
    form->addRow(tr("Ficheros a incluir:"), filesRow);

    connect(filesBrowse, &QPushButton::clicked, this, [this] {
        const QStringList picked =
            QFileDialog::getOpenFileNames(this, tr("Añadir ficheros"),
                                          m_filesEdit->text().isEmpty()
                                              ? QDir::homePath()
                                              : m_filesEdit->text());
        if (picked.isEmpty())
            return;
        QStringList all = splitFiles(m_filesEdit->text());
        all.append(picked);
        m_filesEdit->setText(all.join(QLatin1Char(';')));
    });

    connect(dirBrowse, &QPushButton::clicked, this, [this] {
        const QString dir = QFileDialog::getExistingDirectory(
            this, tr("Añadir carpeta"),
            m_filesEdit->text().isEmpty() ? QDir::homePath() : m_filesEdit->text());
        if (dir.isEmpty())
            return;
        QStringList all = splitFiles(m_filesEdit->text());
        all.append(dir);
        m_filesEdit->setText(all.join(QLatin1Char(';')));
    });

    // --- Formato -------------------------------------------------------------
    m_formatCombo = new QComboBox(this);
    m_formatCombo->setObjectName(QStringLiteral("createFormatCombo"));
    m_formatCombo->addItem(tr("RAR"), QStringLiteral("rar"));
    m_formatCombo->addItem(tr("ZIP"), QStringLiteral("zip"));
    form->addRow(tr("Formato:"), m_formatCombo);

    // --- compresion ----------------------------------------------------------
    m_levelCombo = new QComboBox(this);
    m_levelCombo->setObjectName(QStringLiteral("createLevelCombo"));
    for (int level = 0; level <= 5; ++level)
        m_levelCombo->addItem(levelName(level), level);
    m_levelCombo->setCurrentIndex(3);   // normal
    form->addRow(tr("Compresión:"), m_levelCombo);

    // --- cifrado -------------------------------------------------------------
    m_passwordCombo = new QComboBox(this);
    m_passwordCombo->addItem(tr("No"), QString());
    m_passwordCombo->addItem(tr("Cifrar nombres de archivo"), QStringLiteral("headers"));
    m_passwordCombo->addItem(tr("Cifrar datos"), QStringLiteral("data"));
    form->addRow(tr("Contraseña:"), m_passwordCombo);

    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setObjectName(QStringLiteral("createPasswordEdit"));
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setEnabled(false);
    form->addRow(tr("Contraseña:"), m_passwordEdit);

    m_encryptHeadersCheck = new QCheckBox(tr("Cifrar también los nombres de archivo"), this);
    m_encryptHeadersCheck->setObjectName(QStringLiteral("createEncryptHeadersCheck"));
    form->addRow(QString(), m_encryptHeadersCheck);

    m_solidCheck = new QCheckBox(tr("Archivo sólido (mejor compresión)"), this);
    m_solidCheck->setObjectName(QStringLiteral("createSolidCheck"));
    form->addRow(QString(), m_solidCheck);

    layout->addLayout(form);

    // El tipo de cifrado decide si la casilla de la cabecera tiene sentido: si
    // ya van cifrados los nombres, la casilla estorba.
    connect(m_passwordCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString how = m_passwordCombo->itemData(index).toString();
        m_passwordEdit->setEnabled(!how.isEmpty());
        if (how != QLatin1String("headers"))
            m_encryptHeadersCheck->setEnabled(!how.isEmpty());
    });
    connect(m_encryptHeadersCheck, &QCheckBox::toggled, this, [this](bool on) {
        if (on) {
            const int idx = m_passwordCombo->findData(QStringLiteral("headers"));
            if (idx >= 0)
                m_passwordCombo->setCurrentIndex(idx);
        }
    });
    connect(m_formatCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const bool zip = m_formatCombo->itemData(index).toString() == QLatin1String("zip");
        // El archivo solido solo existe en RAR.
        m_solidCheck->setEnabled(!zip);
        if (zip)
            m_solidCheck->setChecked(false);
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Aceptar"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancelar"));
    connect(buttons, &QDialogButtonBox::accepted, this, [this] {
        QString error;
        if (!validate(&error)) {
            QMessageBox::warning(this, tr("No se puede continuar"), error);
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    syncFormatToExtension();
}

void CreateDialog::syncFormatToExtension()
{
    const QString suffix = QFileInfo(m_pathEdit->text()).suffix().toLower();
    if (suffix == QLatin1String("zip")) {
        m_formatCombo->setCurrentIndex(m_formatCombo->findData(QStringLiteral("zip")));
    } else if (suffix == QLatin1String("rar")) {
        m_formatCombo->setCurrentIndex(m_formatCombo->findData(QStringLiteral("rar")));
    }
}

QString CreateDialog::archivePath() const
{
    return m_pathEdit->text().trimmed();
}

QStringList CreateDialog::files() const
{
    return splitFiles(m_filesEdit->text());
}

int CreateDialog::compressionLevel() const
{
    return m_levelCombo->currentData().toInt();
}

bool CreateDialog::solid() const
{
    return m_solidCheck->isChecked() && chosenFormat() != QLatin1String("zip");
}

bool CreateDialog::encryptHeaders() const
{
    return m_encryptHeadersCheck->isChecked() && chosenFormat() != QLatin1String("zip");
}

bool CreateDialog::hasPassword() const
{
    return !m_passwordEdit->text().isEmpty() && chosenFormat() != QLatin1String("zip");
}

QString CreateDialog::password() const
{
    return m_passwordEdit->text();
}

QString CreateDialog::chosenFormat() const
{
    return m_formatCombo->currentData().toString();
}

void CreateDialog::setFiles(const QStringList &files)
{
    m_filesEdit->setText(files.join(QLatin1Char(';')));
}

void CreateDialog::setFormat(const QString &format)
{
    const int index = m_formatCombo->findData(format);
    if (index >= 0)
        m_formatCombo->setCurrentIndex(index);
}

bool CreateDialog::validate(QString *errorMessage) const
{
    const QString path = archivePath();
    if (path.isEmpty()) {
        *errorMessage = tr("Escribe el nombre del archivo.");
        return false;
    }
    if (files().isEmpty()) {
        *errorMessage = tr("Añade al menos un fichero o carpeta.");
        return false;
    }
    for (const QString &f : files()) {
        if (!QFileInfo::exists(f)) {
            *errorMessage = tr("No existe: %1").arg(f);
            return false;
        }
    }

    const QString suffix = QFileInfo(path).suffix().toLower();
    if (!suffix.isEmpty() && suffix != chosenFormat()) {
        *errorMessage = tr("La extensión del archivo no coincide con el formato elegido.");
        return false;
    }

    if (hasPassword() && password().isEmpty())
        *errorMessage = tr("Has pedido cifrar pero no has escrito la contraseña.");

    if (chosenFormat() == QLatin1String("rar")
        && BinaryLocator::locate().rarPath.isEmpty()) {
        *errorMessage = tr("No se ha encontrado el programa 'rar', que es el único capaz de "
                           "crear archivos RAR. Puedes crear un ZIP en su lugar.");
        return false;
    }
    if (chosenFormat() == QLatin1String("zip")
        && BinaryLocator::locate().sevenZipPath.isEmpty()) {
        *errorMessage = tr("No se ha encontrado 7-Zip, que es el programa que hace los ZIP.");
        return false;
    }
    return true;
}

} // namespace qtrar
