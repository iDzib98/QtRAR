#include "LicenseProbe.h"

#include "BinaryLocator.h"
#include "ProcessRunner.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QStringList>

namespace qtrar {

LicenseProbe::LicenseProbe(QObject *parent) : QObject(parent) {}

LicenseStatus LicenseProbe::probe(const QString &rarPath)
{
    LicenseStatus status;
    const auto T = [](const char *key) { return QCoreApplication::translate("qtrar::LicenseProbe", key); };

    if (rarPath.isEmpty()) {
        status.state = LicenseState::Unknown;
        status.detail = T("El binario 'rar' no esta disponible: no se pueden crear archivos RAR");
        m_status = status;
        emit statusChanged(m_status);
        return status;
    }

    // Busqueda informativa de la clave. Nunca se lee su contenido.
    for (const QString &p : BinaryLocator::licenseKeySearchPaths()) {
        if (QFileInfo::exists(p)) {
            status.keyFileFound = true;
            status.keyFilePath = p;
            break;
        }
    }

    // Sonda: `rar -?` imprime la cabecera sin tocar ningun archivo. En un binario
    // de prueba esa cabecera dice "Trial version"; en uno registrado aparece el
    // nombre del titular. La variante "Evaluation copy. Please register." solo
    // sale en operaciones de escritura, asi que no sirve para sondear.
    ProcessRunner runner;
    const RunResult r = runner.run(rarPath, {QStringLiteral("-?")}, QString(), 5000);
    const QString combined = r.stdOut + QLatin1Char('\n') + r.stdErr;

    const bool nag = combined.contains(QLatin1String("Trial version"), Qt::CaseInsensitive)
        || combined.contains(QLatin1String("Evaluation copy"), Qt::CaseInsensitive)
        || combined.contains(QLatin1String("UNREGISTERED"), Qt::CaseInsensitive);

    if (nag) {
        status.state = LicenseState::Evaluation;
        status.detail = T("RAR en modo de evaluacion (40 dias). "
                          "Crear o modificar archivos RAR requiere licencia de pago.");
    } else {
        status.state = LicenseState::Registered;
        status.detail = T("RAR registrado");
    }

    m_status = status;
    emit statusChanged(m_status);
    return status;
}

QString LicenseProbe::stateText(const LicenseStatus &status)
{
    const auto T = [](const char *key) { return QCoreApplication::translate("qtrar::LicenseProbe", key); };
    switch (status.state) {
    case LicenseState::Evaluation:
        return T("RAR: EVALUACION");
    case LicenseState::Registered:
        return T("RAR: registrado");
    case LicenseState::Unknown:
        break;
    }
    return {};
}

} // namespace qtrar
