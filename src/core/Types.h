// Tipos compartidos del nucleo de QtRAR.
#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QtGlobal>

namespace qtrar {

// Codigos de salida de unrar/rar documentados en rar.txt ("Exit values").
// Ojo: el binario devuelve 0 tambien ante errores no fatales en algunos casos
// (p.ej. un fichero que no es RAR), asi que ExitCode nunca se consulta solo:
// hay que combinarlo con el diagnostico textual. Ver Diagnostics.h.
enum class ExitCode : int {
    Success = 0,
    NonFatalError = 1,
    FatalError = 2,
    ChecksumError = 3,
    LockedArchive = 4,
    WriteError = 5,
    OpenError = 6,
    CommandLineError = 7,
    MemoryError = 8,
    CreateError = 9,
    NoFilesMatched = 10,
    WrongPassword = 11,
    ReadError = 12,
    BadArchive = 13,
    UserBreak = 255,
};

// Tipo de un miembro del archivo, segun el campo "Type:" de `unrar lt`.
enum class EntryType {
    File,
    Directory,
    Symlink,
    Hardlink,
    Other,
};

// Un miembro del archivo tal y como lo devuelve `unrar lt`.
// Todos los campos son opcionales porque unrar omite claves segun el tipo
// (p.ej. los directorios no traen Size ni Packed size).
struct ArchiveEntry {
    QString name;         ///< Ruta interna con '/' como separador, tal cual la devuelve unrar.
    EntryType type = EntryType::File;
    qint64 size = -1;      ///< Bytes sin comprimir; -1 si no viene.
    qint64 packedSize = -1;///< Bytes comprimidos; -1 si no viene (directorios).
    int ratio = -1;        ///< 0-100 tal cual lo reporta unrar; -1 si no viene.
    QDateTime mtime;       ///< Sin coma -> segundos; con coma -> nanosegundos.
    QString attributes;    ///< Atributos crudos, p.ej. "-rw-rw-r--" o "drwxrwxr-x".
    QString crc32;         ///< CRC32 en hexadecimal mayusculas, p.ej. "B9E87FF9".
    QString hostOs;        ///< "Unix" o "Windows".
    QString compression;   ///< Descripcion del algoritmo, p.ej. "RAR 5.0(v50) -m5 -md=128k".
    QStringList flags;     ///< "solid", "encrypted", "directory", "lock"...

    bool isDir() const { return type == EntryType::Directory; }
    bool isEncrypted() const { return flags.contains(QStringLiteral("encrypted")); }
    bool isSolid() const { return flags.contains(QStringLiteral("solid")); }
    bool isLink() const { return type == EntryType::Symlink || type == EntryType::Hardlink; }

    /// Nombre de la carpeta que contiene esta entrada, "" si esta en la raiz.
    QString parentPath() const;
    /// Solo el nombre propio, sin la ruta.
    QString baseName() const;
    /// Ruta interna con separadores nativos del sistema.
    QString nativePath() const;
};

// Datos de cabecera del archivo, previos a la lista de miembros.
struct ArchiveInfo {
    QString path;
    QString format;         ///< "RAR 5", "RAR 4", "ZIP"...
    bool solid = false;
    bool encryptedHeaders = false;
    bool volume = false;    ///< Multivolume.
    int volumeNumber = -1;  ///< -1 si no aplica.
    qint64 volumeOffset = -1;
    qint64 compressedSize = -1;
    qint64 uncompressedSize = -1;

    int fileCount = 0;
    int dirCount = 0;
    qint64 totalSize = 0;      ///< Suma de Size de los ficheros.
    qint64 totalPacked = 0;    ///< Suma de Packed size.
    bool anyEncrypted = false;
    QString comment;         ///< Comentario del archivo, si lo tiene.

    bool isValid() const { return !path.isEmpty(); }
};

// Resultado de listar un archivo: cabecera + miembros.
struct ArchiveListing {
    ArchiveInfo info;
    QVector<ArchiveEntry> entries;

    bool isEmpty() const { return entries.isEmpty(); }
    const ArchiveEntry *find(const QString &name) const;
    /// Nombres de los hijos directos de `dir` ("" para la raiz), en el orden
    /// en que aparecen en el archivo.
    QStringList childrenOf(const QString &dir) const;
};

// Formatos de archivo soportados.
enum class ArchiveFormat {
    Unknown,
    Rar,   ///< RAR 4 o RAR 5
    Zip,
};

// Motor capable de operar un formato concreto.
enum class EngineKind {
    None,
    RarEngine,  ///< unrar (lectura) + rar (escritura, trialware)
    ZipEngine,  ///< 7z (libre, sin limites de tiempo)
};

QString engineKindName(EngineKind kind);
QString archiveFormatName(ArchiveFormat fmt);

} // namespace qtrar
