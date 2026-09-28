#include "StatusPanel.h"

#include "core/HFormat.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QStorageInfo>

namespace qtrar {

StatusPanel::StatusPanel(QWidget *parent) : QWidget(parent)
{
    m_countsLabel = new QLabel(this);
    m_selectionLabel = new QLabel(this);
    m_licenseLabel = new QLabel(this);
    m_freeSpaceLabel = new QLabel(this);
    m_freeSpaceBar = new QProgressBar(this);

    m_freeSpaceLabel->setVisible(false);
    m_freeSpaceBar->setVisible(false);
    m_freeSpaceBar->setMaximum(100);
    m_freeSpaceBar->setMaximumWidth(120);
    m_freeSpaceBar->setTextVisible(false);
    m_freeSpaceBar->setFixedHeight(11);

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 6, 2);
    layout->setSpacing(10);
    layout->addWidget(m_countsLabel);
    layout->addWidget(m_selectionLabel, 1);
    layout->addWidget(m_licenseLabel);
    layout->addWidget(m_freeSpaceLabel);
    layout->addWidget(m_freeSpaceBar);
}

void StatusPanel::setCounts(int files, int folders, qint64 bytes, qint64 packed)
{
    m_countsLabel->setText(HFormat::fileCountText(files, folders, bytes, packed));
}

void StatusPanel::setSelectionInfo(const QString &text)
{
    m_selectionLabel->setText(text);
}

void StatusPanel::setLicenseText(const QString &text)
{
    m_licenseLabel->setText(text);
    m_licenseLabel->setVisible(!text.isEmpty());
}

void StatusPanel::setLicenseWarning(bool warning)
{
    // En modo de evaluacion el aviso se distingue solo por el color, sin
    // molestar con ventanas emerginges.
    m_licenseLabel->setStyleSheet(warning ? QStringLiteral("color: #B06000;")
                                          : QStringLiteral("color: #707070;"));
}

void StatusPanel::setFreeSpace(const QString &path, qint64 total, qint64 free)
{
    if (path.isEmpty() || total <= 0) {
        clearFreeSpace();
        return;
    }
    const int pct = static_cast<int>((free * 100) / total);
    m_freeSpaceBar->setValue(pct);
    m_freeSpaceLabel->setText(tr("Libre en %1: %2")
                                  .arg(path, HFormat::fileSize(free)));
    m_freeSpaceLabel->setVisible(true);
    m_freeSpaceBar->setVisible(true);
}

void StatusPanel::clearFreeSpace()
{
    m_freeSpaceLabel->setVisible(false);
    m_freeSpaceBar->setVisible(false);
}

QString StatusPanel::countsText() const
{
    QStringList parts;
    if (!m_countsLabel->text().isEmpty())
        parts << m_countsLabel->text();
    if (!m_selectionLabel->text().isEmpty())
        parts << m_selectionLabel->text();
    if (!m_licenseLabel->text().isEmpty())
        parts << m_licenseLabel->text();
    if (!m_freeSpaceLabel->text().isEmpty())
        parts << m_freeSpaceLabel->text();
    return parts.join(QStringLiteral("  |  "));
}

} // namespace qtrar
