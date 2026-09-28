// Ejecutor de procesos externos con las defensas necesarias para una GUI.
//
// Decisiones que vienen del comportamiento REAL de unrar 7.23 verificado:
//
//  1. Nunca se omite la clave: si no se pasa -p, unrar imprime "Enter password"
//     y se queda esperando en la terminal. En una GUI eso cuelga la aplicacion.
//     Por eso ProcessRunner exige siempre -p<pwd> o -p- (ver PasswordSwitch).
//
//  2. Se cierra el canal de escritura de stdin al arrancar, para que cualquier
//     lectura interactiva reciba EOF en vez de bloquearse para siempre.
//
//  3. El codigo de salida NO es sufficiente para saber si algo fue un error:
//     - `unrar lt algo.zip`            -> exit 0 + "is not RAR archive"
//     - `unrar rr fichero.rar a b`      -> exit 0 + impresion del uso
//     - `unrar lt noexiste.rar`         -> exit 10
//     Diagnostico centraliza la combinacion de ambos.
#pragma once

#include "Types.h"

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

namespace qtrar {

struct RunResult {
    int exitCode = -1;
    QString stdOut;
    QString stdErr;
    QString stdIn;        ///< Lo que se escribio en stdin.
    bool started = false;
    bool timedOut = false;
    bool cancelled = false;

    bool ok() const { return started && exitCode == 0; }
};

class ProcessRunner : public QObject
{
    Q_OBJECT
public:
    explicit ProcessRunner(QObject *parent = nullptr);
    ~ProcessRunner() override;

    /// Ejecuta `binary` con `args`. Si `stdinData` no es nulo se escribe en
    /// stdin y luego se cierra.
    RunResult run(const QString &binary, const QStringList &args,
                  const QString &stdinData = nullptr, int timeoutMs = 0);

    /// Pide terminar el proceso en curso. Devuelve false si no habia ninguno.
    bool cancel();

    bool isRunning() const;

    /// Construye el switch de clave que evita cualquier prompt interactivo.
    /// Con hasPassword=false genera "-p-", que hace que unrar falle con
    /// "Wrong password" (exit 11) en vez de bloquearse.
    static QString passwordSwitch(bool hasPassword, const QString &password);

signals:
    /// Progreso por fichero. `percent` es -1 si la linea no lo trae.
    void progress(const QString &message, int percent);

private:
    QProcess m_process;
};

/// Analiza las lineas de progreso que emiten rar y unrar:
///   "Adding    ./fichero.txt        14% OK"
///   "Extracting  fichero.txt         45%"
///   "Testing    fichero.txt       OK"
/// Devuelve true si la linea era de progreso y rellena los parametros.
bool parseProgressLine(const QString &line, QString *message, int *percent);

/// Extrae de la salida el mensaje de error legible, ignorando banners y
/// avisos de licencia. Devuelve una cadena vacia si no hay ninguno.
QString extractErrorMessage(const QString &stdErr, const QString &stdOut);

} // namespace qtrar
