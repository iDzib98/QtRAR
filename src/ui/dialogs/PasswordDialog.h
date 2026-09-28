// Dialogo de contrasena. WinRAR lo pide una vez por sesion y ofrece recordarla
// durante la operacion, no guardarla en disco.
#pragma once

#include <QDialog>

class QCheckBox;
class QLineEdit;
class QLabel;

namespace qtrar {

class PasswordDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PasswordDialog(QWidget *parent = nullptr);

    QString password() const;

    /// Mensaje de motivo (p.ej. "Contraseña incorrecta, inténtalo de nuevo").
    void setReason(const QString &reason);

private:
    QLabel *m_reasonLabel;
    QLineEdit *m_edit;
    QCheckBox *m_showBox;
};

} // namespace qtrar
