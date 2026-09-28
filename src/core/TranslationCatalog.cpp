// Catálogo de textos que no puede encontrar `lupdate` por sí solo.
//
//tanto CommandRegistry como Diagnostics traducción textos que eligen en tiempo
// de ejecución (una tabla de acciones y un `switch` de diagnóstico), así que
// no aparecen como literales dentro de un `tr("...")`. Las macros
// QT_TRANSLATE_NOOP de este archivo registran esos textos en el contexto
// correcto para que `lupdate` los extraiga y `lrelease` los comparke.
//
// Este archivo no aporta código: solo existe para el proceso de traducción.
#include "core/Types.h"

namespace qtrar {

namespace {

// --- Context "CommandRegistry": textos de la tabla de acciones -------------
// Deben coincidir exactamente con el campo `textKey` de CommandRegistry y con
// los literales de `CommandRegistry::shortLabel()`. `tools/check-i18n.py`
// avisa si se desincronizan.
[[maybe_unused]] const char *const kCommandTexts[] = {
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Añadir"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Extraer en..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Comprobar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Ver"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Eliminar..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Buscar..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Asistente..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Información"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Buscar &virus..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Comentario..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Proteger archivo..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Crear archivo &autoextraíble..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Reparar..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Abrir"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Extraer en <carpeta>"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Extraer archivos seleccionados"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Archivo nuevo..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Cifrar &nombres de archivo"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Re&nombrar..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Ir a..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Seleccionar &todo"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Deseleccionar todo"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Invertir selección"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Act&ualizar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Opciones..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Personalizar barra de herramientas..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Ingresar licencia de RAR..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Configurar binario &RAR..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Acerca de..."),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "&Ayuda"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Añadir"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Extraer en"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Comprobar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Ver"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Eliminar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Buscar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Asistente"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Información"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Buscar virus"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Comentario"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Proteger"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Auto extraíble"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Reparar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Cifrar nombres"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Abrir"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Actualizar"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Atrás"),
    QT_TRANSLATE_NOOP("qtrar::CommandRegistry", "Subir"),
};


// --- Context "Diagnostics": descripciones de los diagnósticos --------------
[[maybe_unused]] const char *const kDiagnosticTexts[] = {
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Operacion completada"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "El fichero no es un archivo RAR valido"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "No se encuentra el archivo"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Contrasena incorrecta o ausente"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Error de CRC: los datos estan danados"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "El archivo esta bloqueado"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Error de escritura"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Error al abrir el fichero"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Error de lectura"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Archivo danado o con formato desconocido"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Ningun fichero coincide con el patron indicado"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Falta un volumen de la serie"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Memoria insuficiente"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Operacion cancelada"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "La operacion ha tardado demasiado y se ha cancelado"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Se requiere una licencia de pago de RAR para esta operacion"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "Error desconocido"),

    // Mensajes que `analyze()` guarda en ProcessOutcome::message.
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "No se pudo ejecutar el programa externo"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics",
                      "El binario 'rar' esta en modo de evaluacion (40 dias). "
                      "La creacion y modificacion de archivos RAR requiere licencia de pago."),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "El comando no es compatible con esta version de unrar"),
    QT_TRANSLATE_NOOP("qtrar::Diagnostics", "7z no esta disponible: no se pueden abrir archivos ZIP"),
};

// --- Context "LicenseProbe": estado de la licencia de RAR ------------------
[[maybe_unused]] const char *const kLicenseTexts[] = {
    QT_TRANSLATE_NOOP("qtrar::LicenseProbe", "RAR: no encontrado"),
    QT_TRANSLATE_NOOP("qtrar::LicenseProbe", "RAR: desconocido"),
    QT_TRANSLATE_NOOP("qtrar::LicenseProbe", "RAR: EVALUACION"),
    QT_TRANSLATE_NOOP("qtrar::LicenseProbe", "RAR: REGISTRADO"),
};

// --- Context "HFormat": unidades de tamano y fechas ------------------------
[[maybe_unused]] const char *const kFormatTexts[] = {
    QT_TRANSLATE_NOOP("qtrar::HFormat", "bytes"),
    QT_TRANSLATE_NOOP("qtrar::HFormat", "%1 archivos, %2 carpetas"),
};

} // namespace
} // namespace qtrar
