#include "PasswordDialog.h"

#include "ui/ThemeManager.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

namespace qtrar {

PasswordDialog::PasswordDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(tr("Contraseña"));
    setWindowIcon(ThemeManager::icon(QStringLiteral("app")));
    setModal(true);

    auto *root = new QVBoxLayout(this);

    m_reasonLabel = new QLabel(this);
    m_reasonLabel->setWordWrap(true);
    m_reasonLabel->setVisible(false);
    root->addWidget(m_reasonLabel);

    m_edit = new QLineEdit(this);
    m_edit->setEchoMode(QLineEdit::Password);
    m_edit->setPlaceholderText(tr("Contraseña del archivo"));

    auto *form = new QFormLayout;
    form->addRow(tr("&Contraseña:"), m_edit);
    root->addLayout(form);

    m_showBox = new QCheckBox(tr("Mostrar &contraseña"), this);
    root->addWidget(m_showBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons);

    connect(m_showBox, &QCheckBox::toggled, this, [this](bool show) {
        m_edit->setEchoMode(show ? QLineEdit::Normal : QLineEdit::Password);
    });
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_edit, &QLineEdit::returnPressed, this, &QDialog::accept);

    resize(380, 150);
}

QString PasswordDialog::password() const
{
    return m_edit->text();
}

void PasswordDialog::setReason(const QString &reason)
{
    m_reasonLabel->setText(reason);
    m_reasonLabel->setVisible(!reason.isEmpty());
    m_edit->selectAll();
    m_edit->setFocus();
}

} // namespace qtrar
