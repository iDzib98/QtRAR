// Temas e iconos intercambiables.
//
// El usuario puede elegir, de forma independiente:
//   - Tema:    "original" (hocha de estilos propia que imita la aspecto clasico
//              de WinRAR) o "system" (paleta y estilos del escritorio).
//   - Iconos:  "original" (SVG recreados, en res/icons) o "system"
//              (QIcon::fromTheme).
#pragma once

#include <QIcon>
#include <QString>
#include <QWidget>

namespace qtrar {

class ThemeManager
{
public:
    enum class Theme { Original, System };
    enum class IconSet { Original, System };

    /// Carga el tema/iconos desde QSettings y los aplica a la aplicacion.
    /// `app` puede ser nullptr en pruebas.
    static void apply(QWidget *app = nullptr);

    static QIcon icon(const QString &name);
    static void setIconSource(IconSet set);
    static IconSet iconSource();

    static void setTheme(Theme theme, QWidget *app = nullptr);
    static Theme theme();

    /// Indica si el escritorio esta en modo oscuro.
    ///
    /// `QStyleHints::colorScheme()` deberia bastar, pero en varios
    /// compositors de Wayland devuelve `Unknown`, asi que se prueban tambien
    /// los ajustes de GNOME y KDE. Si nada dice nada, se supone claro, que es
    /// lo que se veia siempre.
    static bool systemIsDark();

    /// Vigila el ajuste de color del escritorio y reaplica la paleta cuando
    /// cambia, para que pasar el sistema a oscuro se note sin reiniciar.
    /// Solo tiene efecto con el tema "del sistema".
    static void watchSystemColorScheme(QWidget *app = nullptr);

    /// Aplica paleta oscura o clara segun lo que diga `systemIsDark()`.
    /// Se llama al cambiar de tema, no solo al arrancar.
    static void applyColorScheme(QWidget *app = nullptr);

    /// Devuelve el icono que corresponde a una entrada, para la columna de
    /// nombre: carpeta, fichero, enlace, archivo bloqueado, etc.
    static QIcon entryIcon(const QString &entryName, bool isDirectory, bool encrypted = false);

private:
    static IconSet s_iconSource;
    static Theme s_theme;
};

} // namespace qtrar
