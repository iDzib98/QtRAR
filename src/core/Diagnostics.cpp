#include "Diagnostics.h"

#include <QCoreApplication>
#include <QRegularExpression>

namespace qtrar {

namespace {

bool contains(const QString &text, const char *needle)
{
    return text.contains(QLatin1String(needle), Qt::CaseInsensitive);
}

} // namespace

bool Diagnostics::looksLikeNotAnArchive(const QString &text)
{
    return contains(text, "is not RAR archive")
        || contains(text, "not a RAR archive")
        || contains(text, "Cannot open the file as archive")
        || contains(text, "UNRAR: Unsupported archive format");
}

ProcessOutcome Diagnostics::analyze(const RunResult &result, bool expectContent)
{
    ProcessOutcome out;
    out.exitCode = static_cast<ExitCode>(result.exitCode);

    const QString combined = result.stdErr + QLatin1Char('\n') + result.stdOut;
    out.message = extractErrorMessage(result.stdErr, result.stdOut);

    // 1. Cancelacion y tiempo agotado tienen prioridad sobre todo lo demas.
    if (result.cancelled) {
        out.diagnostic = Diagnostic::Cancelled;
        return out;
    }
    if (result.timedOut) {
        out.diagnostic = Diagnostic::TimedOut;
        return out;
    }
    if (!result.started) {
        out.diagnostic = Diagnostic::OpenError;
        if (out.message.isEmpty())
            out.message = QStringLiteral("No se pudo ejecutar el programa externo");
        return out;
    }

    // 2. Errores de licencia. `rar` imprime este banner antes de oper y,
    //    si no hay licencia pagada, se niega a crear/modificar.
    if (contains(combined, "Evaluation copy")
        || contains(combined, "Please register")
        || contains(combined, "UNREGISTERED")) {
        if (result.exitCode != 0) {
            out.diagnostic = Diagnostic::LicenseRequired;
            if (out.message.isEmpty()) {
                out.message = QStringLiteral(
                    "El binario 'rar' se ha negado a escribir. Durante los 40 dias de "
                    "evaluacion si permite crear y modificar; despues hace falta una "
                    "licencia de pago, que solo compra RARLAB.");
            }
            return out;
        }
    }

    // 3. Casos trampa: exit 0 con contenido que en realidad es un error.
    if (looksLikeNotAnArchive(combined)) {
        out.diagnostic = Diagnostic::NotAnArchive;
        return out;
    }
    if (result.exitCode == 0 && combined.contains(QLatin1String("<Commands>"))
        && expectContent) {
        // El binario imprimio su ayuda: el comando no existe (p.ej. `unrar d`).
        out.diagnostic = Diagnostic::UnknownError;
        if (out.message.isEmpty())
            out.message = QStringLiteral("El comando no es compatible con esta version de unrar");
        return out;
    }

    // 4.b `unrar t` puede terminar con codigo 0 y aun asi imprimir
    //     "Total errors: N". Ese total es la senal autoritativa del resultado
    //     de la prueba, y por eso se consulta antes que los avisos por
    //     fichero: si el proceso dice haber terminado bien, hay danos.
    if (result.exitCode == 0) {
        static const QRegularExpression totalErrors(
            QStringLiteral("Total errors:\\s*(\\d+)"),
            QRegularExpression::CaseInsensitiveOption);
        if (const QRegularExpressionMatch m = totalErrors.match(combined); m.hasMatch()) {
            if (m.captured(1).toInt() > 0) {
                out.diagnostic = Diagnostic::ChecksumError;
                out.message = QStringLiteral("Total errors: %1").arg(m.captured(1));
                return out;
            }
        }
    }

    // 4. Clave incorrecta o ausente.
    if (result.exitCode == static_cast<int>(ExitCode::WrongPassword)
        || contains(combined, "Incorrect password")
        || contains(combined, "Wrong password")) {
        out.diagnostic = Diagnostic::WrongPassword;
        return out;
    }

    // 5. Volumenes.
    if (contains(combined, "Cannot find volume")
        || contains(combined, "Missing volume")
        || contains(combined, "is not the first volume")) {
        out.diagnostic = Diagnostic::VolumeMissing;
        return out;
    }

    // 6. Traduccion directa del codigo de salida.
    switch (static_cast<ExitCode>(result.exitCode)) {
    case ExitCode::Success:
        out.diagnostic = Diagnostic::Success;
        out.success = true;
        return out;
    case ExitCode::ChecksumError:
        out.diagnostic = Diagnostic::ChecksumError;
        return out;
    case ExitCode::LockedArchive:
        out.diagnostic = Diagnostic::LockedArchive;
        return out;
    case ExitCode::WriteError:
        out.diagnostic = Diagnostic::WriteError;
        return out;
    case ExitCode::OpenError:
        out.diagnostic = Diagnostic::OpenError;
        return out;
    case ExitCode::ReadError:
        out.diagnostic = Diagnostic::ReadError;
        return out;
    case ExitCode::BadArchive:
        out.diagnostic = Diagnostic::BadArchive;
        return out;
    case ExitCode::MemoryError:
        out.diagnostic = Diagnostic::OutOfMemory;
        return out;
    case ExitCode::NoFilesMatched:
        // 10 es ambiguo: "no existe el archivo" y "la mascara no coincide".
        if (contains(combined, "Cannot find") || contains(combined, "Cannot open")
            || contains(combined, "not found")
            || contains(combined, "No existe")) {
            out.diagnostic = Diagnostic::FileNotFound;
        } else {
            out.diagnostic = Diagnostic::NoFilesMatched;
        }
        return out;
    case ExitCode::CommandLineError:
        out.diagnostic = Diagnostic::UnknownError;
        return out;
    case ExitCode::WrongPassword:
        // Se ha resuelto antes, en el punto 4, mirando tambien el texto.
        out.diagnostic = Diagnostic::WrongPassword;
        return out;
    case ExitCode::UserBreak:
        out.diagnostic = Diagnostic::Cancelled;
        return out;
    case ExitCode::NonFatalError:
        // 1 = "errores no fatales": la operacion se completo a pesar de avisos.
        out.diagnostic = Diagnostic::Success;
        out.success = true;
        return out;
    case ExitCode::FatalError:
    case ExitCode::CreateError:
        out.diagnostic = Diagnostic::UnknownError;
        return out;
    }

    out.diagnostic = Diagnostic::UnknownError;
    return out;
}

QString Diagnostics::describe(const ProcessOutcome &outcome)
{
    const auto T = [](const char *key) { return QCoreApplication::translate("qtrar::Diagnostics", key); };

    switch (outcome.diagnostic) {
    case Diagnostic::Success:
        return T("Operacion completada");
    case Diagnostic::NotAnArchive:
        return T("El fichero no es un archivo RAR valido");
    case Diagnostic::FileNotFound:
        return T("No se encuentra el archivo");
    case Diagnostic::WrongPassword:
        return T("Contrasena incorrecta o ausente");
    case Diagnostic::ChecksumError:
        return T("Error de CRC: los datos estan danados");
    case Diagnostic::LockedArchive:
        return T("El archivo esta bloqueado");
    case Diagnostic::WriteError:
        return T("Error de escritura");
    case Diagnostic::OpenError:
        return T("Error al abrir el fichero");
    case Diagnostic::ReadError:
        return T("Error de lectura");
    case Diagnostic::BadArchive:
        return T("Archivo danado o con formato desconocido");
    case Diagnostic::NoFilesMatched:
        return T("Ningun fichero coincide con el patron indicado");
    case Diagnostic::VolumeMissing:
        return T("Falta un volumen de la serie");
    case Diagnostic::OutOfMemory:
        return T("Memoria insuficiente");
    case Diagnostic::Cancelled:
        return T("Operacion cancelada");
    case Diagnostic::TimedOut:
        return T("La operacion ha tardado demasiado y se ha cancelado");
    case Diagnostic::LicenseRequired:
        return T("El binario 'rar' no permite esta operacion sin licencia de pago");
    case Diagnostic::ToolNotFound:
        return T("Falta la herramienta externa necesaria para esta operacion");
    case Diagnostic::UnknownError:
    case Diagnostic::None:
        break;
    }
    if (!outcome.message.isEmpty())
        return outcome.message;
    return T("Error desconocido");
}

} // namespace qtrar
