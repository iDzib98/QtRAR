#include "ThemeManager.h"

#include <QApplication>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QFileSystemWatcher>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QRegularExpression>
#include <QSettings>
#include <QStyle>
#include <QStyleHints>

namespace qtrar {

ThemeManager::IconSet ThemeManager::s_iconSource = IconSet::Original;
ThemeManager::Theme ThemeManager::s_theme = Theme::Original;

namespace {
/// Paleta de la plataforma antes de tocarla, para poder volver a ella.
QPalette s_savedPalette;
bool s_savedPaletteIsSet = false;
} // namespace

void ThemeManager::apply(QWidget *app)
{
    QSettings settings;
    const int themeIdx = settings.value(QStringLiteral("ui/theme"), 0).toInt();
    const int iconIdx = settings.value(QStringLiteral("ui/icons"), 0).toInt();
    s_theme = themeIdx == 1 ? Theme::System : Theme::Original;
    s_iconSource = iconIdx == 1 ? IconSet::System : IconSet::Original;
    setTheme(s_theme, app);
}

void ThemeManager::setTheme(Theme theme, QWidget *app)
{
    s_theme = theme;
    if (!qApp)
        return;

    // Con el tema "original" se recarga la hoja de estilos propia. Con el
    // sistema se quita cualquier hoja previa para no heredar el aspecto.
    if (theme == Theme::Original) {
        QFile f(QStringLiteral(":/themes/original.qss"));
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
            qApp->setStyleSheet(QString::fromUtf8(f.readAll()));
    } else {
        qApp->setStyleSheet(QString());
    }
    // El tema "original" es el de siempre, en claro; el del sistema sigue al
    // escritorio, asi que puede ser oscuro.
    if (theme == Theme::System)
        applyColorScheme();
    else if (qApp) {
        qApp->setPalette(s_savedPaletteIsSet ? s_savedPalette : qApp->palette());
        s_savedPaletteIsSet = false;
        if (auto *hints = qApp->styleHints())
            hints->setColorScheme(Qt::ColorScheme::Unknown);
    }
    Q_UNUSED(app)
}

bool ThemeManager::systemIsDark()
{
    if (qApp) {
        // Camino normal: Qt consulta al compositor. Solo se acepta si contesta
        // de verdad, porque hay compositors que devuelven Unknown aunque el
        // escritorio este en oscuro.
        if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Dark)
            return true;
        if (qApp->styleHints()->colorScheme() == Qt::ColorScheme::Light)
            return false;
    }

    // Respaldo para Wayland, donde lo anterior suele fallar.
    const auto gsetting = [](const char *schema, const char *key) -> QString {
        QProcess p;
        // El orden es `gsettings get <esquema> <clave>`; al revés no existe el
        // comando y se pierde la deteccion entera.
        p.start(QStringLiteral("gsettings"),
                {QStringLiteral("get"), QString::fromLatin1(schema), QString::fromLatin1(key)});
        if (!p.waitForFinished(2000))
            return {};
        return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    };

    // GNOME: 'prefer-dark' / 'default'.
    if (const QString v = gsetting("org.gnome.desktop.interface", "color-scheme");
        !v.isEmpty() && v != QLatin1String("'default'")) {
        return v == QLatin1String("'prefer-dark'");
    }
    // KDE plasma: 0 claro, 1 oscuro.
    if (const QString v = gsetting("org.kde.desktopinterface", "color-scheme");
        !v.isEmpty() && v != QLatin1String("No existe el esquema") && !v.contains(QRegularExpression("[a-z]"))) {
        return v.trimmed() == QLatin1String("1");
    }
    return false;
}

namespace {

/// Paleta oscura para Fusion. Los valores son los de Adwaita/KDE oscuros, que
/// es lo que espera cualquiera que tenga el escritorio en ese modo.
QPalette darkPalette()
{
    QPalette p;
    const QColor window(0x24, 0x24, 0x24);
    const QColor base(0x1e, 0x1e, 0x1e);
    const QColor alternate(0x2b, 0x2b, 0x2b);
    const QColor text(0xe6, 0xe6, 0xe6);
    const QColor disabled(0x77, 0x77, 0x77);
    const QColor accent(0x3d, 0x7e, 0xdd);

    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, alternate);
    p.setColor(QPalette::ToolTipBase, window);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::Button, window);
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::red);
    p.setColor(QPalette::Link, accent);
    p.setColor(QPalette::Highlight, accent);
    p.setColor(QPalette::HighlightedText, Qt::white);

    p.setColor(QPalette::PlaceholderText, disabled);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text, disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);

    // Los controles sin foco se dibujan con estos grupos de estados inactivos.
    p.setColor(QPalette::Inactive, QPalette::WindowText, disabled);
    p.setColor(QPalette::Inactive, QPalette::Text, disabled);
    p.setColor(QPalette::Inactive, QPalette::ButtonText, disabled);
    return p;
}

/// Paleta original de la plataforma, guardada para poder volver a ella cuando
/// el usuario cambia de tema en caliente.
} // namespace

namespace {

/// Reaplica la paleta cuando cambia el ajuste de color del escritorio.
///
/// Se vigila el fichero de base de datos de dconf en vez de preguntar a
/// `gsettings` cada poco: lanzar un proceso cada segundo no es gratis, y
/// dconf escribe en ese fichero, asi que el aviso llega igual.
class ColorSchemeWatcher : public QObject
{
public:
    explicit ColorSchemeWatcher(QObject *parent = nullptr)
        : QObject(parent), m_watcher(new QFileSystemWatcher(this))
    {
        connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
                [this](const QString &path) {
                    // dconf sustituye el fichero en vez de escribirlo, asi que
                    // hay que volver a apuntarlo o se pierde la vigilancia.
                    if (!m_watcher->files().contains(path))
                        m_watcher->addPath(path);
                    // Varios cambios seguidos (por ejemplo, cambiar el tema y
                    // los iconos a la vez) se agrupan en uno solo.
                    m_timer.start();
                });
        m_timer.setSingleShot(true);
        m_timer.setInterval(400);
        connect(&m_timer, &QTimer::timeout, this, [this] {
            // Solo tiene sentido si el usuario ha elegido seguir al sistema.
            if (ThemeManager::theme() == ThemeManager::Theme::System)
                ThemeManager::applyColorScheme();
        });

        const QStringList paths = {
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                + QStringLiteral("/dconf/user"),
            QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation)
                + QStringLiteral("/kdeglobals"),
        };
        for (const QString &p : paths) {
            if (QFileInfo::exists(p))
                m_watcher->addPath(p);
        }
    }

private:
    QFileSystemWatcher *m_watcher;
    QTimer m_timer;
};

ColorSchemeWatcher *s_watcher = nullptr;

} // namespace

void ThemeManager::watchSystemColorScheme(QWidget *app)
{
    Q_UNUSED(app)
    if (!qApp || s_watcher)
        return;
    s_watcher = new ColorSchemeWatcher(qApp);
}

void ThemeManager::applyColorScheme(QWidget *app)
{
    if (!qApp)
        return;

    const bool dark = systemIsDark();

    if (dark) {
        if (!s_savedPaletteIsSet) {
            s_savedPalette = qApp->palette();
            s_savedPaletteIsSet = true;
        }
        qApp->setPalette(darkPalette());
    } else if (s_savedPaletteIsSet) {
        // Se vuelve a la paleta de la plataforma en lugar de inventar una clara.
        qApp->setPalette(s_savedPalette);
        s_savedPaletteIsSet = false;
    }

    // Se le dice a Qt el color que ha detectado. Asi los dialogos nativos y los
    // controles que consulten `colorScheme()` siguen la misma linea.
    if (auto *hints = qApp->styleHints()) {
        hints->setColorScheme(dark ? Qt::ColorScheme::Dark : Qt::ColorScheme::Light);
    }
    Q_UNUSED(app)
}

ThemeManager::Theme ThemeManager::theme()
{
    return s_theme;
}

void ThemeManager::setIconSource(IconSet set)
{
    s_iconSource = set;
}

ThemeManager::IconSet ThemeManager::iconSource()
{
    return s_iconSource;
}

QIcon ThemeManager::icon(const QString &name)
{
    if (name.isEmpty())
        return {};

    if (s_iconSource == IconSet::Original) {
        const QIcon ico(QStringLiteral(":/icons/") + name + QStringLiteral(".svg"));
        if (!ico.isNull())
            return ico;
        // Si el SVG no existe todavia, se recurre al tema del sistema en
        // lugar de mostrar un icono vacio.
    }
    return QIcon::fromTheme(name);
}

QIcon ThemeManager::entryIcon(const QString &entryName, bool isDirectory, bool encrypted)
{
    if (isDirectory)
        return icon(encrypted ? QStringLiteral("folder-locked") : QStringLiteral("folder"));

    const QString suffix = QFileInfo(entryName).suffix().toLower();
    const QStringList generic = {QStringLiteral("file"), QStringLiteral("document")};

    // Un subconjunto de tipos, suficiente para los ficheros mas habituales.
    static const QStringList documents = {
        QStringLiteral("txt"), QStringLiteral("doc"), QStringLiteral("docx"),
        QStringLiteral("pdf"), QStringLiteral("rtf"), QStringLiteral("odt"),
    };
    static const QStringList images = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("gif"), QStringLiteral("bmp"), QStringLiteral("svg"),
        QStringLiteral("webp"),
    };
    static const QStringList archives = {
        QStringLiteral("rar"), QStringLiteral("zip"), QStringLiteral("7z"),
        QStringLiteral("tar"), QStringLiteral("gz"), QStringLiteral("bz2"),
    };
    static const QStringList code = {
        QStringLiteral("c"), QStringLiteral("cpp"), QStringLiteral("h"),
        QStringLiteral("py"), QStringLiteral("sh"), QStringLiteral("js"),
        QStringLiteral("json"), QStringLiteral("xml"),
    };
    static const QStringList media = {
        QStringLiteral("mp3"), QStringLiteral("mp4"), QStringLiteral("avi"),
        QStringLiteral("mkv"), QStringLiteral("wav"),
    };

    QString base = generic.last();
    if (documents.contains(suffix)) base = QStringLiteral("document");
    else if (images.contains(suffix)) base = QStringLiteral("image");
    else if (archives.contains(suffix)) base = QStringLiteral("archive");
    else if (code.contains(suffix)) base = QStringLiteral("code");
    else if (media.contains(suffix)) base = QStringLiteral("media");
    else base = QStringLiteral("file");

    if (encrypted)
        return icon(base + QStringLiteral("-locked"));
    return icon(base);
}

} // namespace qtrar
