#include "ProgressDialog.h"

#include "ui/ThemeManager.h"

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QVBoxLayout>

namespace qtrar {

ProgressDialog::ProgressDialog(const QString &title, int totalSteps, QWidget *parent)
    : QDialog(parent), m_totalSteps(totalSteps)
{
    setWindowTitle(title);
    setWindowIcon(ThemeManager::icon(QStringLiteral("app")));
    setModal(true);
    setMinimumWidth(420);

    m_fileLabel = new QLabel(this);
    QFont f = m_fileLabel->font();
    f.setBold(true);
    m_fileLabel->setFont(f);
    m_fileLabel->setWordWrap(true);
    m_fileLabel->setTextFormat(Qt::PlainText);  // los nombres vienen de un archivo externo

    m_statusLabel = new QLabel(this);

    m_bar = new QProgressBar(this);
    m_bar->setRange(0, qMax(1, totalSteps));
    m_bar->setValue(0);
    m_bar->setFormat(QStringLiteral("%p%"));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
    m_cancelButton = buttons->button(QDialogButtonBox::Cancel);
    m_cancelButton->setText(tr("Cancelar"));
    connect(buttons, &QDialogButtonBox::rejected, this, [this] {
        m_cancelled = true;
        m_bar->setEnabled(false);
        m_cancelButton->setEnabled(false);
        m_statusLabel->setText(tr("Cancelando..."));
        emit cancelled();
    });

    auto *root = new QVBoxLayout(this);
    root->addWidget(new QLabel(tr("Procesando..."), this));
    root->addWidget(m_fileLabel);
    root->addWidget(m_bar);
    root->addWidget(m_statusLabel);
    root->addWidget(buttons);
}

void ProgressDialog::setCurrentFile(const QString &name)
{
    m_fileLabel->setText(name);
}

void ProgressDialog::setStatusText(const QString &text)
{
    m_statusLabel->setText(text);
}

void ProgressDialog::setValue(int value)
{
    m_bar->setValue(value);
}

void ProgressDialog::setMaximum(int value)
{
    m_bar->setMaximum(qMax(1, value));
}

void ProgressDialog::advance()
{
    m_bar->setValue(m_bar->value() + 1);
}

int ProgressDialog::value() const
{
    return m_bar->value();
}

void ProgressDialog::closeEvent(QCloseEvent *event)
{
    // Cerrar la ventana es cancelar, igual que el boton.
    if (!m_cancelled) {
        m_cancelled = true;
        emit cancelled();
    }
    QDialog::closeEvent(event);
}

} // namespace qtrar
