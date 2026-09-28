// Ayuda para registrar el binario oficial `rar` sin manipular su clave.
#pragma once

#include "core/LicenseProbe.h"

#include <QDialog>

class QLabel;

namespace qtrar {

class LicenseDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LicenseDialog(const LicenseStatus &status, QWidget *parent = nullptr);
    void setStatus(const LicenseStatus &status);

signals:
    void recheckRequested();

private:
    QLabel *m_statusLabel = nullptr;
    QLabel *m_keyStatusLabel = nullptr;
    QString m_keyPath;
};

} // namespace qtrar
