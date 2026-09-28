// Asistente para vincular el binario oficial `rar` instalado por el usuario.
#pragma once

#include <QDialog>

class QLineEdit;

namespace qtrar {

class RarSetupDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RarSetupDialog(const QString &currentPath, QWidget *parent = nullptr);
    QString binaryPath() const;

protected:
    void accept() override;

private:
    void browse();
    QLineEdit *m_pathEdit = nullptr;
};

} // namespace qtrar
