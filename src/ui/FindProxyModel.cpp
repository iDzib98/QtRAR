#include "FindProxyModel.h"

#include "core/Types.h"
#include "model/ArchiveTreeModel.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QStringList>

namespace qtrar {

FindProxyModel::FindProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void FindProxyModel::setSearchText(const QString &text)
{
    if (m_text == text)
        return;
    m_text = text;
    m_cache.clear();
    invalidateFilter();
}

bool FindProxyModel::matchesPattern(const QString &name, const QString &pattern)
{
    if (pattern.isEmpty())
        return true;

    // Un patron con comodines se traduce a una expresion regular. Se escapan
    // los caracteres de Qt para que `*` y `?` signifiquen lo que signifiquen en
    // WinRAR, y no cualquier cosa.
    if (pattern.contains(QLatin1Char('*')) || pattern.contains(QLatin1Char('?'))) {
        QString regex = QRegularExpression::escape(pattern);
        regex.replace(QStringLiteral("\\*"), QStringLiteral(".*"));
        regex.replace(QStringLiteral("\\?"), QStringLiteral("."));
        return QRegularExpression(regex, QRegularExpression::CaseInsensitiveOption)
            .match(name)
            .hasMatch();
    }

    // Sin comodines: basta con que aparezca dentro del nombre.
    return name.contains(pattern, Qt::CaseInsensitive);
}

bool FindProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (const auto *fs = qobject_cast<const QFileSystemModel *>(sourceModel())) {
        const QModelIndex idx = fs->index(sourceRow, 0, sourceParent);
        if (fs->fileName(idx) == QLatin1String("."))
            return false;
    }
    if (m_text.isEmpty())
        return true;

    const QAbstractItemModel *src = sourceModel();
    const QModelIndex idx = src->index(sourceRow, 0, sourceParent);
    // Se compara con la ruta interna completa (carpeta/nombre): asi
    // "docs/*.txt" tambien encuentra lo que esta dentro de una subcarpeta.
    const QString name = src->data(idx, Qt::UserRole).toString();
    if (name.isEmpty())
        return true;   // la cabecera de la lista se deja siempre

    // Cache por nombre: el filtro se reevalua entero en cada pulsacion y el
    // mismo nombre se pregunta varias veces.
    const auto it = m_cache.constFind(name);
    if (it != m_cache.constEnd())
        return it.value();

    const bool ok = matchesPattern(name, m_text);
    m_cache.insert(name, ok);
    return ok;
}

bool FindProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    if (qobject_cast<const ArchiveTreeModel *>(sourceModel())) {
        const bool aParent = left.data(Qt::UserRole + 4).toBool();
        const bool bParent = right.data(Qt::UserRole + 4).toBool();
        if (aParent != bParent)
            return aParent;
        const bool aDir = left.data(Qt::UserRole + 1).toBool();
        const bool bDir = right.data(Qt::UserRole + 1).toBool();
        if (aDir != bDir)
            return aDir;
        return QString::localeAwareCompare(left.data(Qt::DisplayRole).toString(),
                                           right.data(Qt::DisplayRole).toString()) < 0;
    }
    if (const auto *fs = qobject_cast<const QFileSystemModel *>(sourceModel())) {
        const QFileInfo a = fs->fileInfo(left.siblingAtColumn(0));
        const QFileInfo b = fs->fileInfo(right.siblingAtColumn(0));
        const bool aParent = a.fileName() == QLatin1String("..");
        const bool bParent = b.fileName() == QLatin1String("..");
        if (aParent != bParent)
            return aParent;
        if (a.isDir() != b.isDir())
            return a.isDir();
        return QString::localeAwareCompare(a.fileName(), b.fileName()) < 0;
    }
    return QSortFilterProxyModel::lessThan(left, right);
}

} // namespace qtrar
