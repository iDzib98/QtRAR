#include "ListParser.h"

#include <QHash>
#include <QLocale>
#include <QRegularExpression>

namespace qtrar {

namespace {

// Convierte "2%" a 2 y "113" a 113 sin depender de la configuracion regional.
qint64 toInt64(const QString &s)
{
    QString t = s.trimmed();
    while (t.endsWith(u'%'))
        t.chop(1);
    bool ok = false;
    const qint64 v = t.toLongLong(&ok);
    return ok ? v : -1;
}

EntryType typeFromString(const QString &v)
{
    if (v.compare(QLatin1String("File"), Qt::CaseInsensitive) == 0)
        return EntryType::File;
    if (v.compare(QLatin1String("Directory"), Qt::CaseInsensitive) == 0
        || v.compare(QLatin1String("Dir"), Qt::CaseInsensitive) == 0)
        return EntryType::Directory;
    if (v.contains(QLatin1String("symbolic link"), Qt::CaseInsensitive))
        return EntryType::Symlink;
    if (v.contains(QLatin1String("hard link"), Qt::CaseInsensitive))
        return EntryType::Hardlink;
    return EntryType::Other;
}

} // namespace

bool ListParser::splitKeyValue(const QString &line, QString *key, QString *value)
{
    // La clave va alineada a la derecha, p.ej. " Packed size: 113".
    const QString trimmed = line.trimmed();
    const int colon = trimmed.indexOf(QLatin1Char(':'));
    if (colon <= 0)
        return false;
    *key = trimmed.left(colon).trimmed();
    *value = trimmed.mid(colon + 1).trimmed();
    return !key->isEmpty();
}

QDateTime ListParser::parseUnrarTime(const QString &value)
{
    // "2026-09-25 15:14:30,849861011" o "2026-09-25 15:14:30"
    QString v = value.trimmed();
    int nsec = 0;
    const int comma = v.indexOf(QLatin1Char(','));
    if (comma > 0) {
        QString frac = v.mid(comma + 1);
        v = v.left(comma);
        // Los ultimos digitos son nanosegundos; rellenamos a 9.
        while (frac.size() < 9)
            frac.append(u'0');
        frac = frac.left(9);
        bool ok = false;
        nsec = frac.toInt(&ok);
        if (!ok)
            nsec = 0;
    }
    QDateTime dt = QDateTime::fromString(v, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    if (!dt.isValid())
        return {};
    // fromString sin zona devuelve LocalTime, que es lo que se quiere: unrar
    // escribe la hora local del archivo.
    if (nsec > 0)
        dt = dt.addMSecs(nsec / 1000000);
    return dt;
}

EntryType ListParser::parseType(const QString &value)
{
    return typeFromString(value);
}

ArchiveListing ListParser::parseUnrarTechnical(const QString &output, const QString &archivePath)
{
    ArchiveListing listing;
    ArchiveInfo &info = listing.info;
    info.path = archivePath;

    QVector<ArchiveEntry> entries;
    ArchiveEntry current;
    bool inEntry = false;
    // El comentario del archivo va al principio, como texto libre antes de
    // "Archive:". No tiene clave: se reconoce por la linea que lo introduce.
    static const QString commentHeader = QStringLiteral("Archive comment:");
    // El comentario se acaba cuando empieza la cabecera del archivo. Sin este
    // corte, "Archive:" y todo lo demas se acababa colgando del comentario.
    static const QString archiveHeader = QStringLiteral("Archive:");
    bool inComment = false;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        if (inComment) {
            if (line.startsWith(archiveHeader, Qt::CaseInsensitive)) {
                inComment = false;
            } else {
                info.comment += line;
                info.comment += QLatin1Char('\n');
                continue;
            }
        }
        if (line.compare(commentHeader, Qt::CaseInsensitive) == 0) {
            inComment = true;
            continue;
        }

        // La primera linea es el banner "UNRAR x.y.z freeware Copyright...".
        // Se ignora de forma implicita: no contiene ':' con clave conocida.

        QString key, value;
        if (!splitKeyValue(raw, &key, &value)) {
            // Lineas de error o avisos sueltos ("Incorrect password for x.rar").
            continue;
        }

        if (key.compare(QLatin1String("Name"), Qt::CaseInsensitive) == 0) {
            if (inEntry)
                entries.append(current);
            current = ArchiveEntry();
            current.name = value;
            inEntry = true;
            continue;
        }

        if (!inEntry) {
            // Cabecera del archivo.
            if (key.compare(QLatin1String("Archive"), Qt::CaseInsensitive) == 0) {
                if (info.path.isEmpty())
                    info.path = value;
            } else if (key.compare(QLatin1String("Details"), Qt::CaseInsensitive) == 0) {
                info.format = value.section(QLatin1Char(','), 0, 0).trimmed();
                if (value.contains(QLatin1String("solid"), Qt::CaseInsensitive))
                    info.solid = true;
                if (value.contains(QLatin1String("encrypted headers"), Qt::CaseInsensitive))
                    info.encryptedHeaders = true;
            } else if (key.compare(QLatin1String("Volume number"), Qt::CaseInsensitive) == 0) {
                info.volume = true;
                info.volumeNumber = static_cast<int>(toInt64(value));
            } else if (key.compare(QLatin1String("Volume offset"), Qt::CaseInsensitive) == 0) {
                info.volume = true;
                info.volumeOffset = toInt64(value);
            } else if (key.compare(QLatin1String("Compressed size"), Qt::CaseInsensitive) == 0) {
                info.compressedSize = toInt64(value);
            } else if (key.compare(QLatin1String("Uncompressed size"), Qt::CaseInsensitive) == 0) {
                info.uncompressedSize = toInt64(value);
            }
            continue;
        }

        // Dentro de una entrada.
        if (key.compare(QLatin1String("Type"), Qt::CaseInsensitive) == 0) {
            current.type = typeFromString(value);
        } else if (key.compare(QLatin1String("Size"), Qt::CaseInsensitive) == 0) {
            current.size = toInt64(value);
        } else if (key.compare(QLatin1String("Packed size"), Qt::CaseInsensitive) == 0) {
            current.packedSize = toInt64(value);
        } else if (key.compare(QLatin1String("Ratio"), Qt::CaseInsensitive) == 0) {
            current.ratio = static_cast<int>(toInt64(value));
        } else if (key.compare(QLatin1String("mtime"), Qt::CaseInsensitive) == 0) {
            current.mtime = parseUnrarTime(value);
        } else if (key.compare(QLatin1String("Attributes"), Qt::CaseInsensitive) == 0) {
            current.attributes = value;
        } else if (key.compare(QLatin1String("CRC32"), Qt::CaseInsensitive) == 0) {
            current.crc32 = value;
        } else if (key.compare(QLatin1String("Host OS"), Qt::CaseInsensitive) == 0) {
            current.hostOs = value;
        } else if (key.compare(QLatin1String("Compression"), Qt::CaseInsensitive) == 0) {
            current.compression = value;
        } else if (key.compare(QLatin1String("Flags"), Qt::CaseInsensitive) == 0) {
            // "solid" / "encrypted" / "directory" / "lock", separados por
            // espacios y con espacio final.
            current.flags = value.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        }
    }
    if (inEntry)
        entries.append(current);

    listing.entries = entries;
    while (info.comment.endsWith(QLatin1Char('\n')))
        info.comment.chop(1);

    for (const ArchiveEntry &e : entries) {
        if (e.isDir()) {
            ++info.dirCount;
        } else {
            ++info.fileCount;
            if (e.size > 0)
                info.totalSize += e.size;
            if (e.packedSize > 0)
                info.totalPacked += e.packedSize;
        }
        if (e.isEncrypted())
            info.anyEncrypted = true;
    }
    if (info.compressedSize < 0)
        info.compressedSize = info.totalPacked;
    if (info.uncompressedSize < 0)
        info.uncompressedSize = info.totalSize;

    return listing;
}

QStringList ListParser::parseBareList(const QString &output)
{
    QStringList result;
    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        // Descarta el banner y los avisos de error.
        if (trimmed.contains(QLatin1String("freeware"), Qt::CaseInsensitive))
            continue;
        if (trimmed.startsWith(QLatin1String("UNRAR "), Qt::CaseInsensitive))
            continue;
        result.append(trimmed);
    }
    return result;
}

ArchiveListing ListParser::parseSevenZip(const QString &output, const QString &archivePath)
{
    // Salida de `7z l -slt`:
    //   ----------
    //   Path = /tmp/x.zip
    //   Type = zip
    //   Physical Size = 1234
    //   Headers Size = 200
    //   ----------
    //   Path = readme.txt
    //   Size = 4280
    //   Packed Size = 113
    //   Modified = 2026-09-25 15:14:30
    //   CRC = B9E87FF9
    //   Attributes = A_ -rw-rw-r--
    //   Folder = -
    ArchiveListing listing;
    ArchiveInfo &info = listing.info;
    info.path = archivePath;
    info.format = QStringLiteral("ZIP");

    QVector<ArchiveEntry> entries;
    ArchiveEntry current;
    bool inEntry = false;
    bool headerPending = true; // bloque tras el primer "----------"

    const QStringList lines = output.split(QLatin1Char('\n'));
    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        if (line == QLatin1String("----------")) {
            headerPending = false;
            continue;
        }

        const int eq = line.indexOf(QLatin1Char('='));
        if (eq <= 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString value = line.mid(eq + 1).trimmed();

        if (headerPending) {
            if (key.compare(QLatin1String("Type"), Qt::CaseInsensitive) == 0) {
                if (value.compare(QLatin1String("zip"), Qt::CaseInsensitive) == 0)
                    info.format = QStringLiteral("ZIP");
            } else if (key.compare(QLatin1String("Physical Size"), Qt::CaseInsensitive) == 0) {
                info.compressedSize = toInt64(value);
            }
            continue;
        }

        if (key.compare(QLatin1String("Path"), Qt::CaseInsensitive) == 0) {
            if (inEntry)
                entries.append(current);
            current = ArchiveEntry();
            current.name = value;
            inEntry = true;
        } else if (!inEntry) {
            continue;
        } else if (key.compare(QLatin1String("Size"), Qt::CaseInsensitive) == 0) {
            current.size = toInt64(value);
        } else if (key.compare(QLatin1String("Packed Size"), Qt::CaseInsensitive) == 0) {
            current.packedSize = toInt64(value);
        } else if (key.compare(QLatin1String("Modified"), Qt::CaseInsensitive) == 0) {
            current.mtime = QDateTime::fromString(value, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        } else if (key.compare(QLatin1String("CRC"), Qt::CaseInsensitive) == 0) {
            current.crc32 = value;
        } else if (key.compare(QLatin1String("Attributes"), Qt::CaseInsensitive) == 0) {
            current.attributes = value;
        } else if (key.compare(QLatin1String("Folder"), Qt::CaseInsensitive) == 0) {
            if (value != QLatin1String("-")) {
                current.type = EntryType::Directory;
            }
        }
    }
    if (inEntry)
        entries.append(current);

    listing.entries = entries;
    for (const ArchiveEntry &e : entries) {
        if (e.isDir()) {
            ++info.dirCount;
        } else {
            ++info.fileCount;
            if (e.size > 0)
                info.totalSize += e.size;
            if (e.packedSize > 0)
                info.totalPacked += e.packedSize;
        }
    }
    if (info.compressedSize < 0)
        info.compressedSize = info.totalPacked;
    if (info.uncompressedSize < 0)
        info.uncompressedSize = info.totalSize;
    return listing;
}

} // namespace qtrar
