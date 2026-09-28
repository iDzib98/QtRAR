#include "ui/dialogs/ToolbarDialog.h"

#include "core/HFormat.h"

#include <QCheckBox>
#include <QComboBox>
#include <QAction>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QSettings>
#include <QVBoxLayout>

namespace qtrar {

namespace {

/// Clave de ajustes por comando. El Id es un enum, asi que se guarda el numero
/// para no depender del orden con el que esten declarados los comandos.
QString keyFor(CommandRegistry::Id id)
{
    return QStringLiteral("toolbar/show_") + QString::number(int(id));
}

} // namespace

ToolbarDialog::ToolbarDialog(CommandRegistry *commands, QWidget *parent)
    : QDialog(parent), m_commands(commands)
{
    setWindowTitle(tr("Personalizar la barra de herramientas"));
    setModal(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(tr("Marca los comandos que quieres que aparezcan en la barra:"),
                                 this));

    auto *group = new QGroupBox(tr("Botones"), this);
    auto *groupLayout = new QVBoxLayout(group);
    QSettings settings;
    for (const CommandRegistry::Definition &d : m_commands->toolbarDefinitions()) {
        // El texto se saca de la accion ya construida: `Definition` solo
        // guarda la clave de traduccion, no el texto final.
        QAction *a = m_commands->action(d.id);
        auto *box = new QCheckBox(a ? a->text() : m_commands->definition(d.id)->textKey, group);
        // Por defecto todos los botones de la barra estan visibles, que es lo
        // que viene puesto la primera vez.
        box->setChecked(settings.value(keyFor(d.id), true).toBool());
        groupLayout->addWidget(box);
        m_boxes.insert(int(d.id), box);
    }
    layout->addWidget(group);

    auto *styleGroup = new QGroupBox(tr("Aspecto"), this);
    auto *styleLayout = new QVBoxLayout(styleGroup);
    m_style = new QComboBox(styleGroup);
    // El primero es el de WinRAR: icono grande con el texto debajo.
    m_style->addItem(tr("Icono grande, texto debajo"), int(Qt::ToolButtonTextUnderIcon));
    m_style->addItem(tr("Icono pequeño, texto al lado"), int(Qt::ToolButtonTextBesideIcon));
    m_style->addItem(tr("Solo icono"), int(Qt::ToolButtonIconOnly));
    m_style->addItem(tr("Solo texto"), int(Qt::ToolButtonTextOnly));
    m_style->setCurrentIndex(m_style->findData(
        settings.value(QStringLiteral("toolbar/style"), int(Qt::ToolButtonTextUnderIcon)).toInt()));
    styleLayout->addWidget(m_style);

    m_iconSize = new QComboBox(styleGroup);
    // 32 px es el de WinRAR; los demas son para pantallas pequenas o para
    // quien prefiera la barra discreta.
    m_iconSize->addItem(tr("Grande (32 px)"), 32);
    m_iconSize->addItem(tr("Mediano (24 px)"), 24);
    m_iconSize->addItem(tr("Pequeño (16 px)"), 16);
    m_iconSize->setCurrentIndex(m_iconSize->findData(
        settings.value(QStringLiteral("toolbar/iconSize"), 32).toInt()));
    styleLayout->addWidget(new QLabel(tr("Tamaño del icono:"), styleGroup));
    styleLayout->addWidget(m_iconSize);

    m_movable = new QCheckBox(tr("Permitir mover la barra de sitio"), styleGroup);
    m_movable->setChecked(settings.value(QStringLiteral("toolbar/movable"), true).toBool());
    styleLayout->addWidget(m_movable);
    layout->addWidget(styleGroup);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

bool ToolbarDialog::apply()
{
    QSettings settings;
    bool changed = false;

    for (auto it = m_boxes.constBegin(); it != m_boxes.constEnd(); ++it) {
        QCheckBox *box = it.value();
        const QString key = keyFor(CommandRegistry::Id(it.key()));
        const bool wasVisible = settings.value(key, true).toBool();
        if (wasVisible != box->isChecked()) {
            settings.setValue(key, box->isChecked());
            changed = true;
        }

        // El boton sale de la barra si se desmarca, y vuelve si se marca.
        if (QAction *a = m_commands->action(CommandRegistry::Id(it.key())))
            a->setVisible(box->isChecked());
    }

    const int style = m_style->currentData().toInt();
    if (settings.value(QStringLiteral("toolbar/style"), int(Qt::ToolButtonTextUnderIcon)).toInt()
        != style) {
        settings.setValue(QStringLiteral("toolbar/style"), style);
        changed = true;
    }
    const int iconSize = m_iconSize->currentData().toInt();
    if (settings.value(QStringLiteral("toolbar/iconSize"), 32).toInt() != iconSize) {
        settings.setValue(QStringLiteral("toolbar/iconSize"), iconSize);
        changed = true;
    }
    if (settings.value(QStringLiteral("toolbar/movable"), true).toBool() != m_movable->isChecked()) {
        settings.setValue(QStringLiteral("toolbar/movable"), m_movable->isChecked());
        changed = true;
    }
    return changed;
}

} // namespace qtrar
