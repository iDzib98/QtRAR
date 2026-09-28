#include "CommandRegistry.h"

#include "ui/ThemeManager.h"

#include <QCoreApplication>

#include <algorithm>

namespace qtrar {

CommandRegistry::CommandRegistry(QWidget *parent) : QObject(parent), m_parent(parent)
{
    // El orden de la barra de herramientas es el de WinRAR por defecto: un
    // bloque de izquierda (Anadir, Extraer en, Comprobar, Ver, Eliminar,
    // Buscar, Asistente, Informacion), una linea de separacion, y un bloque
    // de derecha que cambia segun haya un archivo abierto o no.
    // Los dos ultimos numeros de cada fila son `toolbarGroup` y
    // `toolbarVisibility`.
    m_definitions = {
        {CmdAddToArchive,  "&Añadir",              "add",         "Ctrl+A", true,  true,  false, false, false, false, 10, 0, Definition::Always},
        {CmdExtractTo,     "Extraer en...",      "extract",     "Ctrl+E", true,  true,  true,  false, false, false, 20, 0, Definition::Always},
        {CmdTest,          "&Comprobar",          "test",        {},        true,  true,  true,  false, false, false, 30, 0, Definition::Always},
        {CmdView,          "&Ver",           "view",        {},        true,  true,  true,  false, false, false, 40, 0, Definition::Always},
        {CmdDelete,        "&Eliminar...",         "delete",      "Del",     true,  true,  true,  false, false, false, 50, 0, Definition::Always},
        {CmdFind,          "&Buscar...",           "find",        "Ctrl+F",  true,  false, false, true,  false, false, 60, 0, Definition::Always},
        {CmdWizard,        "&Asistente...",         "wizard",      {},        true,  false, false, true,  false, false, 70, 0, Definition::Always},
        {CmdInfo,          "&Información",           "info",        "Ctrl+I",  true,  true,  false, true,  false, false, 80, 0, Definition::Always},
        // Bloque de la derecha con un archivo abierto.
        {CmdVirusScan,     "Buscar &virus...",       "shield",      {},        true,  true,  false, true,  false, false, 90,  1, Definition::WithArchive},
        {CmdComment,       "&Comentario...",        "comment",     {},        true,  true,  true,  true,  false, false, 100, 1, Definition::WithArchive},
        {CmdProtect,       "&Proteger archivo...",        "protect",     {},        true,  true,  false, true,  false, false, 110, 1, Definition::WithArchive},
        {CmdSfx,           "Crear archivo &autoextraíble...",            "sfx",         {},        true,  true,  true,  false, false, false, 120, 1, Definition::WithArchive},
        // Bloque de la derecha sin archivo abierto.
        {CmdRecover,       "&Reparar...",        "repair",      {},        true,  false, false, true,  false, false, 130, 1, Definition::WithoutArchive},

        {CmdOpen,          "&Abrir",           "open",        "Ctrl+O", false, true,  false, false, false, false, 0},
        {CmdExtractHere,   "Extraer en <carpeta>",    "extract",     {},        false, true,  true,  false, false, false, 0},
        {CmdExtractSelected, "Extraer archivos seleccionados", "extract", {},        false, true,  true,  false, false, false, 0},
        {CmdNewArchive,    "&Archivo nuevo...",     "newarchive",  {},        false, true,  false, false, false, false, 0},
        {CmdEncryptNames,  "Cifrar &nombres de archivo",   "encrypt",     {},        false, true,  true,  false, false, false, 0},
        {CmdRename,        "Re&nombrar...",         "rename",      "F2",      false, true,  true,  false, false, false, 0},
        {CmdGoto,          "&Ir a...",           "goto",        "Ctrl+G",  false, true,  false, false, false, false, 0},
        {CmdSelectAll,     "Seleccionar &todo",      "selectall",   "Ctrl+A2", false, true,  true,  false, false, false, 0},
        {CmdDeselectAll,   "&Deseleccionar todo",    "deselectall", {},        false, true,  true,  false, false, false, 0},
        {CmdInvertSelection, "&Invertir selección", "invert",  {},        false, true,  true,  false, false, false, 0},
        {CmdRefresh,       "Act&ualizar",        "refresh",     "F5",      false, false, false, true,  false, false, 0},
        {CmdOptions,       "&Opciones...",        "options",     {},        false, false, false, true,  false, false, 0},
        {CmdCustomizeToolbar, "&Personalizar barra de herramientas...", "options", {},      false, false, false, true,  false, false, 0},
        {CmdConfigureRar,  "Configurar binario &RAR...", "options", {}, false, false, false, true, false, false, 0},
        {CmdAbout,         "&Acerca de...",          "about",       {},        false, false, false, false, true,  false, 0},
        {CmdHelp,          "&Ayuda",           "help",        "F1",      false, false, false, false, true,  false, 0},
        {CmdLicense,       "&Ingresar licencia de RAR...", "info", {}, false, false, false, false, false, true, 0},
    };
}

const CommandRegistry::Definition *CommandRegistry::definition(Id id) const
{
    for (const Definition &d : m_definitions) {
        if (d.id == id)
            return &d;
    }
    return nullptr;
}

QAction *CommandRegistry::createAction(const Definition &def)
{
    if (QAction *existing = m_actions.value(def.id))
        return existing;

    auto *a = new QAction(this);
    a->setObjectName(QString::number(def.id));
    a->setText(tr(def.textKey.toUtf8().constData()));
    a->setIcon(ThemeManager::icon(def.iconName));
    if (!def.shortcut.isEmpty())
        a->setShortcut(QKeySequence(def.shortcut));
    // Bajo el icono va la etiqueta corta; el texto largo, con "..." y "&", se
    // queda en el menu y en el tooltip. Asi queda como en WinRAR.
    const QString shortText = shortLabel(def.id);
    a->setIconText(shortText.isEmpty() ? a->text().remove(QLatin1Char('&')) : shortText);
    a->setToolTip(a->text());
    m_actions.insert(def.id, a);
    if (m_parent)
        m_parent->addAction(a);

    connect(a, &QAction::triggered, this, [this, id = def.id] {
        emit commandTriggered(id);
    });
    return a;
}

QAction *CommandRegistry::action(Id id)
{
    if (QAction *existing = m_actions.value(id))
        return existing;
    const Definition *d = definition(id);
    return d ? createAction(*d) : nullptr;
}

QAction *CommandRegistry::add(Id id)
{
    const Definition *d = definition(id);
    if (!d)
        return nullptr;
    return createAction(*d);
}

QString CommandRegistry::shortLabel(Id id)
{
    // Los atajos con "&" y los "..." sobran bajo el icono: ocupan media barra
    // y no dicen nada que el icono no diga ya. Los textos son los que pone
    // WinRAR en espanol.
    switch (id) {
    case CmdAddToArchive:  return QCoreApplication::translate("qtrar::CommandRegistry", "Añadir");
    case CmdExtractTo:     return QCoreApplication::translate("qtrar::CommandRegistry", "Extraer en");
    case CmdTest:          return QCoreApplication::translate("qtrar::CommandRegistry", "Comprobar");
    case CmdView:          return QCoreApplication::translate("qtrar::CommandRegistry", "Ver");
    case CmdDelete:        return QCoreApplication::translate("qtrar::CommandRegistry", "Eliminar");
    case CmdFind:          return QCoreApplication::translate("qtrar::CommandRegistry", "Buscar");
    case CmdWizard:        return QCoreApplication::translate("qtrar::CommandRegistry", "Asistente");
    case CmdInfo:          return QCoreApplication::translate("qtrar::CommandRegistry", "Información");
    case CmdVirusScan:     return QCoreApplication::translate("qtrar::CommandRegistry", "Buscar virus");
    case CmdComment:       return QCoreApplication::translate("qtrar::CommandRegistry", "Comentario");
    case CmdProtect:       return QCoreApplication::translate("qtrar::CommandRegistry", "Proteger");
    case CmdSfx:           return QCoreApplication::translate("qtrar::CommandRegistry", "Auto extraíble");
    case CmdRecover:       return QCoreApplication::translate("qtrar::CommandRegistry", "Reparar");
    case CmdEncryptNames:  return QCoreApplication::translate("qtrar::CommandRegistry", "Cifrar nombres");
    case CmdOpen:          return QCoreApplication::translate("qtrar::CommandRegistry", "Abrir");
    case CmdRefresh:       return QCoreApplication::translate("qtrar::CommandRegistry", "Actualizar");
    case CmdBack:          return QCoreApplication::translate("qtrar::CommandRegistry", "Atrás");
    case CmdUp:            return QCoreApplication::translate("qtrar::CommandRegistry", "Subir");
    default:               return {};
    }
}

QList<CommandRegistry::Definition> CommandRegistry::toolbarGroupDefinitions(int group) const
{
    QList<Definition> out;
    for (const Definition &d : m_definitions) {
        if (d.inToolbar && d.toolbarGroup == group)
            out.append(d);
    }
    std::sort(out.begin(), out.end(), [](const Definition &a, const Definition &b) {
        return a.toolbarOrder < b.toolbarOrder;
    });
    return out;
}

QList<CommandRegistry::Definition> CommandRegistry::toolbarGroup(int group, bool archiveOpen) const
{
    QList<Definition> out;
    for (const Definition &d : toolbarGroupDefinitions(group)) {
        const bool visible = d.toolbarVisibility == Definition::Always
            || (d.toolbarVisibility == Definition::WithArchive) == archiveOpen;
        if (visible)
            out.append(d);
    }
    return out;
}

QList<CommandRegistry::Definition> CommandRegistry::toolbarDefinitions() const
{
    QList<Definition> out;
    for (const Definition &d : m_definitions) {
        if (d.inToolbar)
            out.append(d);
    }
    std::sort(out.begin(), out.end(), [](const Definition &a, const Definition &b) {
        return a.toolbarOrder < b.toolbarOrder;
    });
    return out;
}

QList<CommandRegistry::Definition> CommandRegistry::menuDefinitions(Menu menu) const
{
    QList<Definition> out;
    for (const Definition &d : m_definitions) {
        bool include = false;
        switch (menu) {
        case MenuFile: include = d.inFileMenu; break;
        case MenuCommands: include = d.inCommandsMenu; break;
        case MenuTools: include = d.inToolsMenu; break;
        case MenuOptions: include = d.inOptionsMenu; break;
        case MenuHelp: include = d.inHelpMenu; break;
        }
        if (include)
            out.append(d);
    }
    return out;
}

void CommandRegistry::setArchiveOpen(bool open)
{
    struct Rule {
        Id id;
        bool needsArchive;
    };
    // Solo estas acciones sobreviven sin un archivo abierto. Las de navegacion
    // se gestionan por separado en MainWindow.
    static const Rule rules[] = {
        {CmdExtractTo, true}, {CmdExtractHere, true}, {CmdExtractSelected, true},
        {CmdTest, true}, {CmdView, true}, {CmdDelete, true}, {CmdFind, true},
        {CmdWizard, true}, {CmdInfo, true}, {CmdVirusScan, true},
        {CmdProtect, true}, {CmdComment, true}, {CmdEncryptNames, true},
        {CmdSfx, true}, {CmdAddToArchive, true}, {CmdRename, true},
        {CmdGoto, true}, {CmdSelectAll, true}, {CmdDeselectAll, true},
        {CmdInvertSelection, true}, {CmdNewArchive, false}, {CmdOpen, false},
        // "Anadir" y "Reparar" se pueden usar sin archivo abierto: anaden a
        // uno nuevo, o reparan el que se elija.
        {CmdAddToArchive, false}, {CmdRecover, false},
    };
    for (const Rule &r : rules) {
        if (QAction *a = m_actions.value(r.id))
            a->setEnabled(!r.needsArchive || open);
    }
}

void CommandRegistry::setCanCreateArchives(bool can)
{
    // Añadir nunca se deshabilita: si falta rar, su acción abre el asistente
    // para localizar el binario oficial. El resto sí depende de que RAR pueda
    // crear/modificar archivos.
    for (Id id : {CmdNewArchive, CmdWizard, CmdSfx, CmdEncryptNames,
                  CmdProtect, CmdDelete, CmdRename}) {
        if (QAction *a = m_actions.value(id)) {
            // El estado base se guarda aparte: combinarlo con `a->isEnabled()`
            // haria que una vez deshabilitada no pudiera volver a habilitarse
            // aunque despues se CONSEguiese una licencia.
            const bool base = m_baseEnabled.value(id, a->isEnabled());
            m_baseEnabled.insert(id, base);
            a->setEnabled(base && can);
        }
    }
}

} // namespace qtrar
