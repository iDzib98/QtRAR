// Dialogo para elegir que botones se ven en la barra de herramientas.
//
// WinRAR abre un arbol con todos los comandos y permite marcar/desmarcar cada
// uno, y ademas alternar entre "con icono" y "con icono y texto". Qt no trae un
// modo de personalizacion equivalente, asi que se hace a mano con una lista de
// comprobacion: el resultado se guarda en QSettings y la ventana lo aplica.
#pragma once

#include "ui/CommandRegistry.h"

#include <QDialog>
#include <QHash>

class QCheckBox;
class QComboBox;

namespace qtrar {

class ToolbarDialog : public QDialog
{
    Q_OBJECT
public:
    ToolbarDialog(CommandRegistry *commands, QWidget *parent = nullptr);

    /// Aplica lo elegido y lo guarda en QSettings. Devuelve true si cambio algo
    /// (para que la ventana decida si hace falta refrescar).
    bool apply();

private:
    /// No es `const`: `CommandRegistry::action()` crea la accion si todavia
    /// no existe, y aqui se necesitan todas.
    CommandRegistry *m_commands;
    QHash<int, QCheckBox *> m_boxes;   ///< CommandRegistry::Id -> casilla.
    QComboBox *m_style = nullptr;
    QComboBox *m_iconSize = nullptr;
    QCheckBox *m_movable = nullptr;
};

} // namespace qtrar
