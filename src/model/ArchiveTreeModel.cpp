#include "ArchiveTreeModel.h"

#include "core/HFormat.h"
#include "ui/ThemeManager.h"

#include <QBrush>
#include <QDir>
#include <QFont>
#include <QIcon>

#include <algorithm>

namespace qtrar {

QString ArchiveTreeModel::headerText(int column)
{
    switch (column) {
    case ColumnName: return tr("Nombre");
    case ColumnSize: return tr("Tamaño");
    case ColumnPacked: return tr("Comprimido");
    case ColumnType: return tr("Tipo");
    case ColumnModified: return tr("Modificado");
    case ColumnCrc32: return tr("CRC32");
    default: return {};
    }
}

ArchiveTreeModel::ArchiveTreeModel(QObject *parent) : QAbstractItemModel(parent) {}

void ArchiveTreeModel::setListing(const ArchiveListing &listing)
{
    beginResetModel();
    m_listing = listing;
    m_currentPath.clear();
    // La busqueda pertenece al archivo anterior: sin esto, abrir otro
    // archivo dejaba el modelo en modo busqueda con el patron viejo.
    m_searchPattern.clear();
    m_searchMode = false;
    buildLevel();
    endResetModel();
}

void ArchiveTreeModel::setCurrentPath(const QString &path)
{
    if (m_currentPath == path)
        return;
    // Una ruta que no existe dentro del archivo dejaria la vista vacia con una
    // direccion inventada en la barra, que es peor que no moverse: se ignora.
    if (!isInsideArchive(path))
        return;
    beginResetModel();
    // Navegar cancela la busqueda: si no, el usuario haria clic en una carpeta
    // y seguiria viendo una lista a plano que ya no corresponde a donde esta.
    m_searchMode = false;
    m_searchPattern.clear();
    m_currentPath = path;
    buildLevel();
    endResetModel();
}

bool ArchiveTreeModel::isInsideArchive(const QString &path) const
{
    // La raiz se representa con la cadena vacia, y "subir" desde el primer
    // nivel vuelve ahi, asi que vacia significa raiz, no "ruta invalida".
    if (path.isEmpty() || path == QLatin1String(".") || path == QLatin1String("/"))
        return true;

    const QString want = path;
    for (const ArchiveEntry &e : m_listing.entries) {
        const QString name = e.name;
        if (name == want)
            return e.type == EntryType::Directory;
        // Tambien vale cualquier carpeta que sea prefijo de alguna entrada: asi
        // se puede escribir a mano una carpeta que unrar no listo explicitamente.
        if (name.startsWith(want + QLatin1Char('/')))
            return true;
    }
    return false;
}

void ArchiveTreeModel::setSearchPattern(const QString &pattern, Matcher matches)
{
    beginResetModel();
    m_searchPattern = pattern;
    m_searchMode = !pattern.isEmpty() && matches != nullptr;
    if (m_searchMode)
        buildSearchLevel(matches);
    else
        buildLevel();
    endResetModel();
}

void ArchiveTreeModel::clearSearchPattern()
{
    if (!m_searchMode && m_searchPattern.isEmpty())
        return;
    setSearchPattern(QString(), nullptr);
}

void ArchiveTreeModel::buildSearchLevel(const Matcher &matches)
{
    m_nodes.clear();
    m_rowByPath.clear();

    for (int i = 0; i < m_listing.entries.size(); ++i) {
        const QString &path = m_listing.entries.at(i).name;
        // Se busca en el nombre del fichero, no en la ruta: buscar "notas"
        // tiene que encontrar "src/docs/notas.txt" sin tener que escribir
        // "src/docs/notas".
        const QString base = path.section(QLatin1Char('/'), -1);
        if (!matches(base) && !matches(path))
            continue;

        Node node;
        node.name = path;      // a plano se ve la ruta entera
        node.fullPath = path;
        node.entryIndex = i;
        m_rowByPath.insert(path, m_nodes.size());
        m_nodes.append(node);
    }

    // En una busqueda no se navega por carpetas: se pulsa la entrada y se abre
    // su carpeta, que es lo que hace WinRAR con "buscar y saltar al archivo".
    m_currentPath.clear();
}

QString ArchiveTreeModel::parentPath() const
{
    if (m_currentPath.isEmpty())
        return {};
    const int slash = m_currentPath.lastIndexOf(u'/');
    return slash < 0 ? QString() : m_currentPath.left(slash);
}

void ArchiveTreeModel::refresh()
{
    const QString keep = m_currentPath;
    beginResetModel();
    buildLevel();
    m_currentPath = keep;
    endResetModel();
}

QVector<ArchiveTreeModel::Node> ArchiveTreeModel::childrenOf(const QString &dir) const
{
    QVector<Node> result;
    if (m_listing.info.path.isEmpty())
        return result;
    QHash<QString, int> rowByPath;

    QString prefix = dir;
    if (!prefix.isEmpty() && !prefix.endsWith(u'/'))
        prefix += u'/';
    const int prefixLen = prefix.size();

    // Crea entradas hijas directas y, si el listado solo contiene rutas
    // anidadas (habitual en ZIP y en algunos RAR), sintetiza sus carpetas
    // intermedias. Si mas adelante aparece la carpeta explicita se convierte
    // el nodo virtual en uno enlazado a su ArchiveEntry.
    for (int i = 0; i < m_listing.entries.size(); ++i) {
        const ArchiveEntry &e = m_listing.entries.at(i);
        if (!e.name.startsWith(prefix))
            continue;
        const QString rest = e.name.mid(prefixLen);
        if (rest.isEmpty())
            continue;
        const int slash = rest.indexOf(u'/');
        if (slash >= 0) {
            const QString childName = rest.left(slash);
            if (childName.isEmpty())
                continue;
            const QString childPath = prefix + childName;
            if (!rowByPath.contains(childPath)) {
                rowByPath.insert(childPath, result.size());
                result.append({childName, childPath, -1});
            }
            continue;
        }

        const auto existing = rowByPath.constFind(e.name);
        if (existing != rowByPath.constEnd()) {
            Node &node = result[existing.value()];
            if (node.entryIndex < 0 && e.isDir())
                node.entryIndex = i;
            continue;
        }
        rowByPath.insert(e.name, result.size());
        result.append({rest, e.name, i});
    }

    // WinRAR pone siempre las carpetas delante de los ficheros.
    std::stable_sort(result.begin(), result.end(), [this](const Node &a, const Node &b) {
        const bool aDir = a.entryIndex < 0 || m_listing.entries.at(a.entryIndex).isDir();
        const bool bDir = b.entryIndex < 0 || m_listing.entries.at(b.entryIndex).isDir();
        if (aDir != bDir)
            return aDir;
        return false; // estable: respeta el orden del archivo
    });
    const int slash = dir.lastIndexOf(u'/');
    const QString parent = slash < 0 ? QString() : dir.left(slash);
    result.prepend({QStringLiteral(".."), parent, -1, true});
    return result;
}

void ArchiveTreeModel::buildLevel()
{
    m_nodes = childrenOf(m_currentPath);
    m_rowByPath.clear();
    for (int i = 0; i < m_nodes.size(); ++i) {
        if (!m_nodes.at(i).parentEntry)
            m_rowByPath.insert(m_nodes.at(i).fullPath, i);
    }
}

QModelIndex ArchiveTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() || row < 0 || row >= m_nodes.size() || column < 0
        || column >= ColumnCount) {
        return {};
    }
    return createIndex(row, column, quintptr(row + 1));
}

QModelIndex ArchiveTreeModel::parent(const QModelIndex &child) const
{
    // Vista plana: cada nivel es una lista independiente, sin nodos reales
    // detras de las filas. Es como se comporta WinRAR al navegar.
    Q_UNUSED(child)
    return {};
}

int ArchiveTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_nodes.size();
}

int ArchiveTreeModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : ColumnCount;
}

const ArchiveEntry *ArchiveTreeModel::entryAt(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_nodes.size())
        return nullptr;
    const int entryIndex = m_nodes.at(index.row()).entryIndex;
    if (entryIndex < 0 || entryIndex >= m_listing.entries.size())
        return nullptr;
    return &m_listing.entries.at(entryIndex);
}

bool ArchiveTreeModel::isDirectory(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_nodes.size())
        return false;
    const Node &node = m_nodes.at(index.row());
    if (node.parentEntry)
        return true;
    const int entryIndex = node.entryIndex;
    return entryIndex < 0 || (entryIndex < m_listing.entries.size()
                              && m_listing.entries.at(entryIndex).isDir());
}

bool ArchiveTreeModel::isParentEntry(const QModelIndex &index) const
{
    return index.isValid() && index.row() >= 0 && index.row() < m_nodes.size()
        && m_nodes.at(index.row()).parentEntry;
}

QString ArchiveTreeModel::entryPath(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_nodes.size())
        return {};
    return m_nodes.at(index.row()).fullPath;
}

QString ArchiveTreeModel::entryName(const QModelIndex &index) const
{
    if (!index.isValid() || index.row() >= m_nodes.size())
        return {};
    return m_nodes.at(index.row()).name;
}

QModelIndex ArchiveTreeModel::indexForPath(const QString &path, int column) const
{
    const auto it = m_rowByPath.constFind(path);
    if (it == m_rowByPath.constEnd())
        return {};
    return index(it.value(), column);
}

QVariant ArchiveTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_nodes.size())
        return {};
    const Node &node = m_nodes.at(index.row());
    const ArchiveEntry *e = entryAt(index);
    const bool directory = isDirectory(index);

    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case ColumnName:
            return node.name;
        case ColumnSize:
            return e ? HFormat::fileSizeOrEmpty(e->size) : QString();
        case ColumnPacked:
            return e ? HFormat::fileSizeOrEmpty(e->packedSize) : QString();
        case ColumnType:
            if (directory)
                return tr("Carpeta");
            if (!e)
                return {};
            switch (e->type) {
            case EntryType::Symlink: return tr("Enlace simbolico");
            case EntryType::Hardlink: return tr("Enlace duro");
            case EntryType::Other: return tr("Desconocido");
            case EntryType::File: break;
            case EntryType::Directory: break;
            }
            // WinRAR distingue archivo de "documento" por extension; aqui se
            // distingue por el sistema anfitrion del archivo.
            return e->hostOs.compare(QLatin1String("Unix"), Qt::CaseInsensitive) == 0
                ? tr("Archivo") : tr("Documento");
        case ColumnModified:
            return e ? HFormat::fileDateTime(e->mtime) : QString();
        case ColumnCrc32:
            return e ? e->crc32 : QString();
        default:
            return {};
    }

    case Qt::DecorationRole:
        if (index.column() == ColumnName)
            return ThemeManager::entryIcon(node.name, directory, e && e->isEncrypted());
        return {};

    case Qt::FontRole:
        if (index.column() == ColumnName && directory) {
            QFont f;
            f.setBold(true);
            return f;
        }
        return {};

    case Qt::TextAlignmentRole:
        if (index.column() == ColumnSize || index.column() == ColumnPacked)
            return int(Qt::AlignRight | Qt::AlignVCenter);
        return int(Qt::AlignLeft | Qt::AlignVCenter);

    case Qt::UserRole:      // ruta interna completa, para seleccion y acciones
        return node.fullPath;
    case Qt::UserRole + 1:  // booleano de directorio
        return directory;
    case Qt::UserRole + 2:  // bytes sin comprimir
        return e ? e->size : -1;
    case Qt::UserRole + 3:  // bytes comprimidos
        return e ? e->packedSize : -1;
    case Qt::UserRole + 4:  // fila especial `..`
        return node.parentEntry;

    default:
        break;
    }
    return {};
}

QVariant ArchiveTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal)
        return QAbstractItemModel::headerData(section, orientation, role);
    if (role == Qt::DisplayRole)
        return headerText(section);
    if (role == Qt::TextAlignmentRole
        && (section == ColumnSize || section == ColumnPacked)) {
        return int(Qt::AlignRight | Qt::AlignVCenter);
    }
    return {};
}

Qt::ItemFlags ArchiveTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

} // namespace qtrar
