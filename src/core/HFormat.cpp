#include "HFormat.h"

#include <QCoreApplication>
#include <QLocale>

namespace qtrar {

namespace {

QString group(int value)
{
    return QLocale().toString(value);
}

/// Los textos de este archivo los compone un `switch` o una concatenacion, asi
/// que `lupdate` no los ve: se traducen a mano con el contexto de HFormat, el
/// mismo que usa `TranslationCatalog.cpp` para registrarlos.
QString T(const char *text)
{
    return QCoreApplication::translate("qtrar::HFormat", text);
}

} // namespace

QString HFormat::fileSize(qint64 bytes, bool withSeparator)
{
    if (bytes < 0)
        return {};

    const QLocale loc;
    const double kb = 1024.0;

    if (bytes < kb) {
        if (withSeparator)
            return QString::number(bytes);
        return QString::number(bytes);
    }
    if (bytes < kb * kb)
        return loc.toString(bytes / kb, 'f', (bytes < kb * 10) ? 2 : 1) + QLatin1String(" KB");
    if (bytes < kb * kb * kb)
        return loc.toString(bytes / (kb * kb), 'f', (bytes < kb * kb * 10) ? 2 : 1)
            + QLatin1String(" MB");
    if (bytes < kb * kb * kb * kb)
        return loc.toString(bytes / (kb * kb * kb), 'f', 2) + QLatin1String(" GB");
    return loc.toString(bytes / (kb * kb * kb * kb), 'f', 2) + QLatin1String(" TB");
}

QString HFormat::fileSizeOrEmpty(qint64 bytes)
{
    if (bytes < 0)
        return {};
    return fileSize(bytes);
}

QString HFormat::fileDateTime(const QDateTime &dt)
{
    if (!dt.isValid())
        return {};
    return dt.toString(QStringLiteral("dd.MM.yy HH:mm"));
}

QString HFormat::fileDateTimeLong(const QDateTime &dt)
{
    if (!dt.isValid())
        return {};
    return dt.toString(QStringLiteral("dd/MM/yyyy HH:mm:ss"));
}

QString HFormat::percent(int value)
{
    if (value < 0)
        return {};
    return QString::number(value) + QLatin1Char('%');
}

QString HFormat::fileCountText(int files, int folders, qint64 bytes, qint64 packed)
{
    const QString f = T("%1 archivos, %2 carpetas").arg(group(files)).arg(group(folders));
    if (bytes < 0)
        return f;
    QString s = f + QStringLiteral(" (%1").arg(fileSize(bytes));
    if (packed >= 0 && packed != bytes)
        s += QStringLiteral(", %1").arg(fileSize(packed));
    s += QLatin1Char(')');
    return s;
}

QString HFormat::compactNumber(qint64 value)
{
    if (value < 0)
        return {};
    const QLocale loc;
    if (value < 1024)
        return loc.toString(value);
    if (value < 1024 * 1024)
        return loc.toString(value / 1024.0, 'f', 0) + QLatin1String(" K");
    if (value < 1024LL * 1024 * 1024)
        return loc.toString(value / (1024.0 * 1024), 'f', 1) + QLatin1String(" M");
    return loc.toString(value / (1024.0 * 1024 * 1024), 'f', 1) + QLatin1String(" G");
}

} // namespace qtrar
