// Detecta el estado de la licencia del binario `rar`.
//
// `rar` imprime "Evaluation copy. Please register." en su salida durante los
// 40 dias de prueba (EULA rarlab 2). QtRAR no oculta ese banner: lo refleja en
// la barra de estado, como hace WinRAR, y mientras dure la prueba se puede
// seguir trabajando, incluido crear y modificar archivos. Lo que no se permite
// es saltarse la licencia: sin binario `rar` no hay escritura.
//
// IMPORTANTE: QtRAR nunca lee, copia ni escribe rarreg.key. Solo informa de si
// el binario parece encontrar una, porque es el propio `rar` quien la usa.
#pragma once

#include "Types.h"

#include <QObject>
#include <QString>

namespace qtrar {

enum class LicenseState {
    Unknown,        ///< No se pudo determinar (rar no esta instalado).
    Evaluation,     ///< Modo prueba: se puede usar, caduca a los 40 dias.
    Registered,     ///< El binario no muestra el banner de prueba.
};

struct LicenseStatus {
    LicenseState state = LicenseState::Unknown;
    QString detail;
    bool keyFileFound = false;   ///< Existe rarreg.key en una ruta conocida.
    QString keyFilePath;         ///< Solo la ruta, nunca el contenido.

    /// Se puede escribir cuando hay un binario `rar` utilizable. Comprobado
    /// con RAR 7.23: en modo evaluation `rar a` funciona durante los 40 dias de
    /// prueba, asi que bloquearlo seria peor que el problema que evita. Solo se
    /// rechaza cuando no se sabe si el binario sirve; entonces el usuario pierde
    /// la opcion, pero nunca se le rompe la operacion a medias.
    bool canCreateArchives() const
    {
        return state != LicenseState::Unknown;
    }
};

class LicenseProbe : public QObject
{
    Q_OBJECT
public:
    explicit LicenseProbe(QObject *parent = nullptr);

    /// Interroga al binario `rar`. `rarPath` vacio -> Unknown.
    /// `isVersionProbe` usa -iver, que es la invocacion mas barata posible.
    LicenseStatus probe(const QString &rarPath);

    static QString stateText(const LicenseStatus &status);

signals:
    void statusChanged(const LicenseStatus &status);

private:
    LicenseStatus m_status;
};

} // namespace qtrar
