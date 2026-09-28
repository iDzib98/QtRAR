#include "LicenseDialog.h"

#include "core/BinaryLocator.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QGroupBox>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace qtrar {

LicenseDialog::LicenseDialog(const LicenseStatus &status, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Licencia de RAR"));
    setModal(true);
    resize(560, 440);

    auto *layout = new QVBoxLayout(this);
    m_statusLabel = new QLabel(this);
    m_statusLabel->setObjectName(QStringLiteral("rarLicenseStatus"));
    m_statusLabel->setWordWrap(true);
    layout->addWidget(m_statusLabel);

    m_keyStatusLabel = new QLabel(this);
    m_keyStatusLabel->setObjectName(QStringLiteral("rarLicenseKeyStatus"));
    m_keyStatusLabel->setWordWrap(true);
    layout->addWidget(m_keyStatusLabel);

    auto *instructions = new QLabel(
        tr("Para registrar RAR, adquiere una licencia en RARLAB y coloca el archivo "
           "<code>rarreg.key</code> en una de las ubicaciones que busca RAR. Después, "
           "pulsa «Comprobar de nuevo». QtRAR no lee, copia ni escribe el contenido de la clave; "
           "es el propio RAR quien la valida."), this);
    instructions->setWordWrap(true);
    instructions->setTextFormat(Qt::RichText);
    layout->addWidget(instructions);

    auto *locationsBox = new QGroupBox(tr("Ubicaciones que comprueba RAR"), this);
    auto *locationsLayout = new QVBoxLayout(locationsBox);
    auto *locations = new QListWidget(locationsBox);
    locations->setObjectName(QStringLiteral("rarLicenseLocations"));
    locations->setSelectionMode(QAbstractItemView::NoSelection);
    for (const QString &path : BinaryLocator::licenseKeySearchPaths())
        locations->addItem(path);
    locationsLayout->addWidget(locations);
    layout->addWidget(locationsBox, 1);

    auto *buttons = new QDialogButtonBox(this);
    auto *openLocation = buttons->addButton(tr("Abrir carpeta"), QDialogButtonBox::ActionRole);
    auto *buy = buttons->addButton(tr("RARLAB"), QDialogButtonBox::ActionRole);
    auto *recheck = buttons->addButton(tr("Comprobar de nuevo"), QDialogButtonBox::ActionRole);
    buttons->addButton(tr("Cerrar"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);

    connect(openLocation, &QPushButton::clicked, this, [this] {
        // Abrir una carpeta para que el usuario gestione el archivo manualmente;
        // QtRAR no lo copia, inspecciona ni modifica.
        const QString folder = m_keyPath.isEmpty() ? QDir::homePath()
                                                   : QFileInfo(m_keyPath).absolutePath();
        QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
    });
    connect(buy, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.rarlab.com/")));
    });
    connect(recheck, &QPushButton::clicked, this, &LicenseDialog::recheckRequested);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    setStatus(status);
}

void LicenseDialog::setStatus(const LicenseStatus &status)
{
    m_keyPath = status.keyFileFound ? status.keyFilePath : QString();
    switch (status.state) {
    case LicenseState::Evaluation:
        m_statusLabel->setText(tr("<b>Modo de evaluación</b><br>RARLAB permite crear y modificar "
                                  "archivos durante los 40 días de prueba. Después se requiere "
                                  "una licencia de pago."));
        break;
    case LicenseState::Registered:
        m_statusLabel->setText(tr("<b>RAR parece estar registrado</b><br>El binario no muestra "
                                  "el aviso de evaluación."));
        break;
    case LicenseState::Unknown:
        m_statusLabel->setText(tr("<b>No se pudo comprobar la licencia</b><br>Instala el binario "
                                  "oficial <code>rar</code> para comprobar el estado."));
        break;
    }

    if (status.keyFileFound) {
        m_keyStatusLabel->setText(
            tr("Se encontró <code>rarreg.key</code> en:<br>%1<br>QtRAR solo comprueba que existe; "
               "RAR valida su contenido.").arg(status.keyFilePath.toHtmlEscaped()));
    } else {
        m_keyStatusLabel->setText(
            tr("No se encontró <code>rarreg.key</code> en las ubicaciones indicadas."));
    }
}

} // namespace qtrar
