#include "Types.h"

namespace qtrar {

const ArchiveEntry *ArchiveListing::find(const QString &name) const
{
    for (const ArchiveEntry &e : entries) {
        if (e.name == name)
            return &e;
    }
    return nullptr;
}

QStringList ArchiveListing::childrenOf(const QString &dir) const
{
    QString prefix = dir;
    if (!prefix.isEmpty() && !prefix.endsWith(u'/'))
        prefix += u'/';

    QStringList result;
    const int prefixLen = prefix.size();
    for (const ArchiveEntry &e : entries) {
        if (!e.name.startsWith(prefix))
            continue;
        const QString rest = e.name.mid(prefixLen);
        if (rest.isEmpty() || rest.contains(u'/'))
            continue; // el propio directorio o algo mas profundo
        result.append(rest);
    }
    return result;
}

QString ArchiveEntry::parentPath() const
{
    const int slash = name.lastIndexOf(u'/');
    return slash < 0 ? QString() : name.left(slash);
}

QString ArchiveEntry::baseName() const
{
    const int slash = name.lastIndexOf(u'/');
    return slash < 0 ? name : name.mid(slash + 1);
}

QString ArchiveEntry::nativePath() const
{
    return QString(name).replace(u'/', QLatin1Char('/'));
}

QString engineKindName(EngineKind kind)
{
    switch (kind) {
    case EngineKind::RarEngine: return QStringLiteral("RAR");
    case EngineKind::ZipEngine: return QStringLiteral("ZIP");
    case EngineKind::None: break;
    }
    return QStringLiteral("—");
}

QString archiveFormatName(ArchiveFormat fmt)
{
    switch (fmt) {
    case ArchiveFormat::Rar: return QStringLiteral("RAR");
    case ArchiveFormat::Zip: return QStringLiteral("ZIP");
    case ArchiveFormat::Unknown: break;
    }
    return QStringLiteral("—");
}

} // namespace qtrar
