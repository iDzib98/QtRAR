// Fachada de alto nivel sobre los binarios externos.
//
// Reparto de responsabilidades, segun lo que los binarios hacen de verdad:
//
//   LECTURA   unrar   -> libre, sin limite de tiempo. No lee ZIP y no sabe
//                       borrar ni renombrar (exit 7 / impresion del uso).
//   ESCRITURA rar     -> trialware de 40 dias. Obligatorio para crear, borrar,
//                       renombrar y comentar. NO puede crear ZIP.
//   ZIP       7z      -> libre, sin limite. Necesario porque WinRAR abre ZIP y
//                       la CLI de Linux no; tambien es el plan B de creacion.
//
// Ver src/core/README-arquitectura.md y el comentario de Types.h.
#pragma once

#include "BinaryLocator.h"
#include "Diagnostics.h"
#include "ProcessRunner.h"
#include "Types.h"

#include <QObject>
#include <QStringList>

namespace qtrar {

/// Opciones de extraccion, en los terminos del dialogo de WinRAR.
enum class ExtractUpdateMode { Replace, Update, Freshen };
enum class ExistingFilesMode { Confirm, Overwrite, Skip, Rename };

struct ExtractOptions {
    QString destination;
    QStringList items;        ///< Rutas internas a extraer; vacio = todo.
    bool keepBrokenFiles = false;  ///< -kb
    ExistingFilesMode existingFiles = ExistingFilesMode::Overwrite;
    ExtractUpdateMode updateMode = ExtractUpdateMode::Replace;
    bool extractToStdout = false;  ///< -e frente a -x
    QString password;
    bool hasPassword = false;
    bool preservePaths = true;     ///< si false, equivalente a -ep
};

/// Opciones de creacion/añadido.
struct CreateOptions {
    QString archivePath;
    QStringList files;       ///< Ficheros o carpetas del sistema a añadir.
    int compressionLevel = 3;///< -m0..-m5
    bool storeOnly = false;  ///< -m0 implicito
    int dictionarySizeMb = 32; ///< -md
    bool solid = false;      ///< -s
    bool createVolumes = false;
    QString volumeSize;      ///< p.ej. "100m"
    QString password;
    bool hasPassword = false;
    bool encryptHeaders = false; ///< -hp en lugar de -p
    bool sfx = false;
    bool addRecoveryRecord = false; ///< -rr
    bool updateExisting = true;     ///< -u frente a -a (crea o actualiza)
};

class ArchiveService : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveService(QObject *parent = nullptr);

    void setTools(const ExternalTools &tools) { m_tools = tools; }
    const ExternalTools &tools() const { return m_tools; }

    /// Detecta el formato leyendo la firma del fichero. No lanza unrar.
    static ArchiveFormat detectFormat(const QString &path);

    /// Devuelve true si existe algun motor capaz de leer ese formato.
    static bool isSupported(const QString &path);
    static bool isSupported(ArchiveFormat fmt);

    /// Listado tecnico completo (cabecera + todos los miembros).
    /// synchronous y seguro para llamar desde un hilo worker.
    ArchiveListing list(const QString &archivePath, const QString &password = {},
                        bool hasPassword = false, ProcessOutcome *outcome = nullptr) const;

    /// Nombres de los hijos directos de `subPath` dentro del archivo.
    QStringList listDirectory(const QString &archivePath, const QString &subPath,
                              const QString &password, bool hasPassword,
                              ProcessOutcome *outcome = nullptr) const;

    /// Prueba la integridad de todo el archivo o de `items`.
    ProcessOutcome test(const QString &archivePath, const QStringList &items = {},
                        const QString &password = {}, bool hasPassword = false) const;

    /// Extrae. Es la unica operacion de QtRAR que puede tardar mucho.
    ProcessOutcome extract(const QString &archivePath, const ExtractOptions &options) const;

    /// Crea un archivo nuevo o añade ficheros a uno existente.
    /// Devuelve LicenseRequired si `rar` no tiene licencia valida.
    ProcessOutcome create(const CreateOptions &options) const;

    /// Crea un ZIP con 7z. Es el plan B de creacion, siempre disponible.
    ProcessOutcome createZip(const CreateOptions &options) const;

    /// Borra miembros. Solo `rar` puede hacerlo.
    ProcessOutcome removeItems(const QString &archivePath, const QStringList &items) const;

    /// Lee el comentario del archivo. Va aparte porque el listado normal usa
    /// `-c-` (sin comentarios) para no traer texto que casi nadie mira, y solo
    /// hace falta cuando el usuario pide la ficha o el comentario.
    QString readComment(const QString &archivePath) const;

    /// Guarda `commentFile` como comentario del archivo. Solo `rar` puede:
    /// `unrar` no tiene forma de escribirlo. Ojo: `-z` toma el nombre PEGADO
    /// (`-zcomentario.txt`); con un espacio, `rar` se come el nombre del
    /// comentario y lo toma por el del archivo.
    ProcessOutcome setComment(const QString &archivePath, const QString &commentFile) const;

    /// Renombra un miembro. Solo `rar` puede hacerlo.
    ProcessOutcome renameItem(const QString &archivePath, const QString &from,
                              const QString &to) const;

    /// Anade el registro de recuperacion (`rar rr`) a un archivo RAR ya
    /// existente. Es lo que WinRAR llama "proteger archivo": guarda una copia
    /// de las cabeceras para poder reconstruirlas si el archivo se dana.
    ProcessOutcome addRecoveryRecord(const QString &archivePath) const;

    /// Repara un archivo RAR usando su registro de recuperacion (`rar rc`).
    /// Es lo que WinRAR llama "Reparar": reconstruye lo que se pueda a partir
    /// de la copia de las cabeceras que dejo "Proteger". `rar` no avisa de los
    /// errores que repara, asi que despues conviene comprobar el archivo.
    ProcessOutcome recoverArchive(const QString &archivePath) const;

    /// Reempaqueta el archivo cifrando tambien los nombres (`-hp`). No hay un
    /// comando de `rar` que lo haga en sitio: hay que extraer y volver a crear,
    /// asi que primero se extrae a un temporal y solo se sustituye el original
    /// si el nuevo archivo se ha creado bien. Devuelve la ruta del temporal en
    /// `stagedPath` para que la interfaz pueda mostrarlo.
    ProcessOutcome encryptFileNames(const QString &archivePath, const QString &password,
                                    const QString &items, QString *stagedPath) const;

    /// Comprueba si la serie de volumenes esta completa.
    bool volumesPresent(const QString &archivePath, QString *missingVolume = nullptr) const;

    /// Primera parte de la serie a la que pertenece `archivePath` (o el mismo
    /// camino si el archivo no forma parte de una serie).
    QString firstVolumeOf(const QString &archivePath) const;

signals:
    void progress(const QString &message, int percent);

private:
    ArchiveFormat engineFor(const QString &archivePath) const;
    QStringList buildListArgs(const QString &archivePath, const QString &password,
                              bool hasPassword) const;

    ExternalTools m_tools;
    mutable ProcessRunner m_runner;
};

} // namespace qtrar
