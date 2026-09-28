// Traduce el resultado de un proceso externo en un diagnostico util.
//
// Motivo de existir: los codigos de salida de unrar 7.23 NO son fiables por si
// solos. Comprobado empiricamente:
//
//   unrar lt -p- -y -c- fichero.zip   -> exit 0, texto "is not RAR archive"
//   unrar lt -p- -y -c- /etc/hostname -> exit 0, texto "is not RAR archive"
//   unrar rr -p- -y -c- a.rar b c     -> exit 0, imprime el uso del programa
//   unrar lt -p- -y -c- noexiste.rar  -> exit 10 ("no se encontraron ficheros")
//   unrar lt -p- -y -c- cifrado.rar   -> exit 11 + "Incorrect password for ..."
//
// Conclusion: hay que combinar siempre codigo + texto. Esta clase es el unico
// sitio donde se toma esa decision, para que ningun consumidor se equivoque.
#pragma once

#include "ProcessRunner.h"
#include "Types.h"

#include <QString>

namespace qtrar {

enum class Diagnostic {
    None,
    Success,
    NotAnArchive,      ///< El fichero existe pero no es del formato esperado.
    FileNotFound,      ///< No existe el archivo.
    WrongPassword,     ///< Cabecceras o datos cifrados, clave incorrecta o ausente.
    ChecksumError,     ///< CRC fallido: datos danados.
    LockedArchive,     ///< Archivo bloqueado con el comando k.
    WriteError,
    OpenError,
    ReadError,
    BadArchive,
    NoFilesMatched,    ///< La mascara no selecciono nada (no siempre es un error).
    VolumeMissing,     ///< Falta un volumen de la serie.
    OutOfMemory,
    Cancelled,
    TimedOut,
    LicenseRequired,   ///< `rar` se nego a escribir por licencia.
    ToolNotFound,      ///< Falta la herramienta externa (rar / 7z) para la operacion.
    UnknownError,
};

struct ProcessOutcome {
    Diagnostic diagnostic = Diagnostic::Success;
    ExitCode exitCode = ExitCode::Success;
    QString message;       ///< Mensaje legible extraido de la salida.
    bool success = false;

    /// Indica si conviene pedir una clave al usuario y reintentar.
    bool needsPassword() const { return diagnostic == Diagnostic::WrongPassword; }
    /// Indica si el fallo se debe a la licencia de pago de `rar`.
    bool needsLicense() const { return diagnostic == Diagnostic::LicenseRequired; }
};

class Diagnostics
{
public:
    /// Analiza una salida de unrar/rar. `expectContent` indica si la operacion
    /// debia devolver datos (listar, probar) y por tanto un exit 0 sin texto
    /// con errores implicaria fallo.
    static ProcessOutcome analyze(const RunResult &result, bool expectContent = false);

    /// Traduce un diagnostico a texto localizable, con sus parametros.
    static QString describe(const ProcessOutcome &outcome);

    /// Comprueba si una salida indica que el fichero no es un RAR.
    static bool looksLikeNotAnArchive(const QString &text);
};

} // namespace qtrar
