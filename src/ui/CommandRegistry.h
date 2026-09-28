// Registro central de comandos de la aplicacion.
//
// Un unico sitio donde vive cada accion: identificador, texto, icono, atajo,
// si esta disponible y quien la ejecuta. Los menus, la barra de herramientas,
// el menu contextual y el dialogo de personalizacion se generan a partir de
// aqui, que es lo que permite completar el clon (Asistente, SFX, comentario,
// proteger, virus, busqueda) sin tocar la ventana principal.
#pragma once

#include <QAction>
#include <QHash>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>

class QMenu;

namespace qtrar {

class CommandRegistry : public QObject
{
    Q_OBJECT
public:
    /// Identificadores estables. Se guardan en las preferencias del usuario
    /// (barra de herramientas y atajos), asi que no deben renombrarse.
    enum Id {
        CmdOpen = 1000,      ///< Abrir archivo
        CmdExtractTo,        ///< Extraer en...
        CmdExtractHere,      ///< Extraer en <carpeta>
        CmdExtractSelected,  ///< Extraer archivos seleccionados
        CmdTest,             ///< Probar
        CmdView,             ///< Ver
        CmdOpenFile,         ///< Abrir archivo extraido
        CmdDelete,           ///< Eliminar...
        CmdFind,             ///< Buscar...
        CmdWizard,           ///< Asistente de creacion
        CmdInfo,             ///< Informacion
        CmdVirusScan,        ///< Analisis antivirus (gancho)
        CmdProtect,          ///< Proteger archivo (gancho)
        CmdComment,          ///< Comentario
        CmdEncryptNames,     ///< Cifrar nombres de archivo
        CmdSfx,              ///< Crear autoextraible
        CmdAddToArchive,     ///< Anadir al archivo...
        CmdNewArchive,       ///< Crear archivo nuevo...
        CmdRename,           ///< Renombrar
        CmdGoto,             ///< Ir a...
        CmdBack,             ///< Atras
        CmdUp,               ///< Subir un nivel
        CmdRefresh,          ///< Actualizar
        CmdSelectAll,        ///< Seleccionar todo
        CmdDeselectAll,      ///< Deseleccionar todo
        CmdInvertSelection,  ///< Invertir seleccion
        CmdOptions,          ///< Opciones
        CmdAbout,            ///< Acerca de
        CmdHelp,             ///< Ayuda
        CmdCustomizeToolbar, ///< Personalizar barra de herramientas
        CmdRecover,          ///< Reparar un archivo con su registro de recuperacion
        CmdLicense,          ///< Estado y gestion manual de la licencia de RAR
        CmdConfigureRar,     ///< Seleccionar el binario oficial rar
    };

    struct Definition {
        Id id;
        QString textKey;    ///< Clave de traduccion (sin tr(), se resuelve al construir).
        /// Etiqueta corta para la barra, como en WinRAR: el texto del menu
        /// lleva "..." y "&", que bajo el icono estorban ("Eliminar..." no es
        /// lo que pone WinRAR, es "Eliminar"). Vacio = se usa `textKey`.
        QString iconName;   ///< Nombre en res/icons, sin extension.
        QString shortcut;   ///< Secuencia de Qt, p.ej. "Ctrl+E".
        bool inToolbar = false;
        bool inFileMenu = false;
        bool inCommandsMenu = false;
        bool inToolsMenu = false;
        bool inOptionsMenu = false;
        bool inHelpMenu = false;
        int toolbarOrder = 0;
        /// Grupo de la barra: 0 = bloque de la izquierda, 1 = bloque de la
        /// derecha, el que va tras la linea de separacion. En WinRAR el
        /// bloque derecho cambia segun haya un archivo abierto o no.
        int toolbarGroup = 0;
        /// Cuando se ve el boton: `Always` esta siempre, `WithArchive` solo
        /// con un archivo abierto y `WithoutArchive` solo sin el. El bloque
        /// derecho necesita las tres: "Reparar" es el caso exclusivo.
        enum ToolbarVisibility { Always, WithArchive, WithoutArchive };
        ToolbarVisibility toolbarVisibility = Always;
    };

    explicit CommandRegistry(QWidget *parent = nullptr);

    /// Devuelve la accion, creandola a la primera peticion. Asi los menus, la
    /// barra y el menu contextual pueden pedir cualquier accion por su id sin
    /// que la ventana principal tenga que registrarlas una a una.
    QAction *action(Id id);
    QAction *createAction(const Definition &def);

    /// Etiqueta corta que va bajo el icono en la barra de herramientas, como
    /// en WinRAR ("Eliminar", no "Eliminar..."). Se devuelve ya traducida.
    /// Se registra para `lupdate` con QT_TRANSLATE_NOOP en
    /// `core/TranslationCatalog.cpp`.
    static QString shortLabel(Id id);
    const Definition *definition(Id id) const;

    /// Devuelve el QAction recien creado (creandolo si hace falta) con texto,
    /// icono y atajo. La activacion siempre se emite como `commandTriggered`,
    /// de modo que la ventana principal no necesita slots por accion.
    QAction *add(Id id);

    enum Menu { MenuFile, MenuCommands, MenuTools, MenuOptions, MenuHelp };
    QList<Definition> toolbarDefinitions() const;
    QList<Definition> toolbarGroupDefinitions(int group) const;
    /// Botones del grupo indicado segun el estado: sin archivo abierto sale
    /// "Reparar", y con archivo abierto "Buscar virus", "Comentario",
    /// "Proteger" y "Auto extraible", como en WinRAR.
    QList<Definition> toolbarGroup(int group, bool archiveOpen) const;
    QList<Definition> menuDefinitions(Menu menu) const;

    /// Conecta la disponibilidad de las acciones con la llegada de un archivo
    /// abierto: sin archivo abierto, casi todo lo destructivo se deshabilita.
    void setArchiveOpen(bool open);

    /// Habilita solo las acciones que pueden ejecutarse con la licencia actual.
    void setCanCreateArchives(bool can);

signals:
    void commandTriggered(qtrar::CommandRegistry::Id id);

private:
    QWidget *m_parent;
    /// Estado de habilitacion anterior a la licencia, por accion.
    QHash<Id, bool> m_baseEnabled;
    QHash<int, QAction *> m_actions;
    QList<Definition> m_definitions;
};

} // namespace qtrar
