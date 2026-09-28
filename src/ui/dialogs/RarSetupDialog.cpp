#include "RarSetupDialog.h"

#include "core/BinaryLocator.h"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

namespace qtrar {

RarSetupDialog::RarSetupDialog(const QString &currentPath, QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Configurar el binario RAR"));
    setModal(true);
    resize(560, 240);

    auto *layout = new QVBoxLayout(this);
    auto *message = new QLabel(
        tr("Para crear y modificar archivos RAR, QtRAR necesita el ejecutable oficial "
           "<code>rar</code> de RARLAB. Descarga el paquete de Linux, extráelo y selecciona "
           "el ejecutable <code>rar</code> (no <code>unrar</code>). QtRAR guardará la ruta "
           "para las próximas ejecuciones; no descarga ni redistribuye ese binario."), this);
    message->setWordWrap(true);
    message->setTextFormat(Qt::RichText);
    layout->addWidget(message);

    auto *pathRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit(currentPath, this);
    m_pathEdit->setObjectName(QStringLiteral("rarBinaryPath"));
    m_pathEdit->setPlaceholderText(tr("Ruta al ejecutable rar"));
    auto *browseButton = new QPushButton(tr("Examinar…"), this);
    pathRow->addWidget(m_pathEdit, 1);
    pathRow->addWidget(browseButton);
    layout->addLayout(pathRow);

    auto *download = new QPushButton(tr("Descargar desde RARLAB…"), this);
    layout->addWidget(download, 0, Qt::AlignLeft);

    auto *buttons = new QDialogButtonBox(this);
    buttons->addButton(tr("Cancelar"), QDialogButtonBox::RejectRole);
    auto *use = buttons->addButton(tr("Usar este binario"), QDialogButtonBox::AcceptRole);
    use->setDefault(true);
    layout->addWidget(buttons);

    connect(browseButton, &QPushButton::clicked, this, &RarSetupDialog::browse);
    connect(download, &QPushButton::clicked, this, [] {
        QDesktopServices::openUrl(QUrl(QStringLiteral("https://www.rarlab.com/download.htm")));
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &RarSetupDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString RarSetupDialog::binaryPath() const
{
    return QFileInfo(m_pathEdit->text().trimmed()).absoluteFilePath();
}

void RarSetupDialog::browse()
{
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Seleccionar el ejecutable rar"), m_pathEdit->text(),
        tr("Ejecutable RAR (rar);;Todos los archivos (*)"));
    if (!path.isEmpty())
        m_pathEdit->setText(path);
}

void RarSetupDialog::accept()
{
    const QFileInfo info(m_pathEdit->text().trimmed());
    if (!info.exists() || !info.isFile() || !info.isExecutable()) {
        QMessageBox::warning(this, tr("Binario RAR no válido"),
                             tr("Selecciona un archivo ejecutable llamado rar."));
        return;
    }
    if (BinaryLocator::versionOf(info.absoluteFilePath(), QStringLiteral("-iver")).isEmpty()) {
        QMessageBox::warning(this, tr("No se reconoce el binario"),
                             tr("El ejecutable seleccionado no parece ser RAR para Linux."));
        return;
    }
    QDialog::accept();
}

} // namespace qtrar
