// Modelo jerarquico del contenido de un archivo, con las columnas y el
// comportamiento de WinRAR: carpetas primero y en negrita, rutas internas
// intactas, ordenacion por columna y navegacion por rutas internas.
#pragma once

#include "core/Types.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QVector>
#include <functional>

namespace qtrar {

class ArchiveTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    enum Column {
        ColumnName = 0,
        ColumnSize,
        ColumnPacked,
        ColumnType,
        ColumnModified,
        ColumnCrc32,
        ColumnCount,
    };

    // Identificadores de la cabecera de WinRAR, guardados para poder
    // reordenar/ocultar columnas desde las opciones.
    static QString headerText(int column);

    explicit ArchiveTreeModel(QObject *parent = nullptr);

    /// Reemplaza todo el contenido y situa la vista en la raiz del archivo.
    void setListing(const ArchiveListing &listing);

    /// Cambia la carpeta mostrada. La ruta es interna, con '/'.
    void setCurrentPath(const QString &path);
    QString currentPath() const { return m_currentPath; }

    /// Indica si `path` es una carpeta real del archivo (o la raiz).
    /// Una ruta inventada se rechaza en vez de dejar la vista en blanco.
    bool isInsideArchive(const QString &path) const;

    void refresh();

    /// Modo busqueda. Con un patron puesto, la vista deja de mostrar una
    /// carpeta y pasa a listar, a plano, TODAS las entradas del archivo cuyo
    /// nombre coincide. Sin esto, buscar solo miraria el nivel visible y no
    /// habria forma de encontrar lo que esta en una subcarpeta.
    /// `matches` es el predicado; el modelo no sabe de comodines, se lo pasan.
    using Matcher = std::function<bool(const QString &)>;
    void setSearchPattern(const QString &pattern, Matcher matches);
    void clearSearchPattern();
    bool inSearchMode() const { return m_searchMode; }
    QString searchPattern() const { return m_searchPattern; }

    ArchiveListing listing() const { return m_listing; }
    ArchiveInfo info() const { return m_listing.info; }
    QString archivePath() const { return m_listing.info.path; }
    bool hasArchive() const { return !m_listing.info.path.isEmpty(); }

    /// Ruta interna completa de la entrada del indice indicado.
    QString entryPath(const QModelIndex &index) const;
    /// Ruta interna de la entrada, con los nombres de los ancestros.
    QString entryName(const QModelIndex &index) const;
    QModelIndex indexForPath(const QString &path, int column = ColumnName) const;

    /// Devuelve null para una carpeta virtual sintetizada a partir de rutas.
    const ArchiveEntry *entryAt(const QModelIndex &index) const;
    /// True tambien para carpetas virtuales inferidas de rutas como `a/b.txt`.
    bool isDirectory(const QModelIndex &index) const;
    /// La fila especial `..`, que vuelve al directorio anterior.
    bool isParentEntry(const QModelIndex &index) const;

    /// Camino de un nivel hacia arriba. Devuelve "" en la raiz.
    QString parentPath() const;

    // QAbstractItemModel
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    // Nodo del arbol visible. Solo se materializa el nivel actual mas sus
    // entradas completas en m_entries: es lo que permite navegar sin releer
    // el archivo (necesario para los archivos solidos).
    struct Node {
        QString name;      ///< Nombre propio, no la ruta completa.
        QString fullPath;  ///< Ruta interna completa.
        int entryIndex = -1; ///< Indice en m_listing.entries, -1 si es carpeta virtual.
        bool parentEntry = false; ///< Fila especial `..`, no es un miembro del archivo.
    };

    void buildLevel();
    QVector<Node> childrenOf(const QString &dir) const;

    void buildSearchLevel(const Matcher &matches);

    ArchiveListing m_listing;
    QString m_currentPath;
    QString m_searchPattern;
    bool m_searchMode = false;
    QVector<Node> m_nodes;   ///< Nodos del nivel visible.
    QHash<QString, int> m_rowByPath;
};

} // namespace qtrar
