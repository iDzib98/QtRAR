#include "BinaryLocator.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QSettings>

namespace qtrar {

namespace {

QString firstExisting(const QStringList &candidates)
{
    for (const QString &c : candidates) {
        if (c.isEmpty())
            continue;
        const QFileInfo fi(c);
        if (fi.exists() && fi.isFile() && fi.isExecutable())
            return fi.absoluteFilePath();
    }
    return {};
}

QString locateOne(const QString &envVar, const QString &name,
                  const QString &appDir, const QString &projectBin,
                  const QString &configuredPath = {})
{
    QStringList candidates;

    const QString fromEnv = qEnvironmentVariable(envVar.toLatin1().constData());
    if (!fromEnv.isEmpty())
        candidates << fromEnv;
    if (!configuredPath.isEmpty())
        candidates << configuredPath;

    const QString base = appDir.isEmpty() ? QCoreApplication::applicationDirPath() : appDir;
    if (!base.isEmpty()) {
        candidates << base + u'/' + name
                   << base + QStringLiteral("/bin/") + name
                   << base + QStringLiteral("/libexec/") + name
                   << base + QStringLiteral("/../bin/") + name
                   << base + QStringLiteral("/../lib/qtrar/") + name;
    }
    if (!projectBin.isEmpty())
        candidates << projectBin + u'/' + name;

    const QString found = firstExisting(candidates);
    if (!found.isEmpty())
        return found;

    // Ultimo recurso: el PATH del sistema.
    return QStandardPaths::findExecutable(name);
}

QString projectBinDir()
{
    // Solo tiene sentido cuando se ejecuta desde el arbol de fuentes.
    const QString appDir = QCoreApplication::applicationDirPath();
    for (const QString &rel : {QStringLiteral(".."), QStringLiteral("../..")}) {
        const QString dir = QFileInfo(appDir + u'/' + rel).absoluteFilePath();
        if (QFileInfo::exists(dir + QStringLiteral("/CMakeLists.txt"))
            && QFileInfo::exists(dir + QStringLiteral("/bin"))) {
            return dir + QStringLiteral("/bin");
        }
    }
    return {};
}

} // namespace

ExternalTools BinaryLocator::locate(const QString &appDir)
{
    ExternalTools t;
    const QString projectBin = projectBinDir();
    const QString configuredRar = QSettings().value(QStringLiteral("tools/rarPath")).toString();

    t.unrarPath = locateOne(QStringLiteral("QTRAR_UNRAR"), QStringLiteral("unrar"), appDir, projectBin);
    t.rarPath = locateOne(QStringLiteral("QTRAR_RAR"), QStringLiteral("rar"), appDir, projectBin,
                          configuredRar);
    t.sevenZipPath = locateOne(QStringLiteral("QTRAR_7Z"), QStringLiteral("7z"), appDir, QString());

    if (t.sevenZipPath.isEmpty()) {
        for (const QString &alt : {QStringLiteral("7za"), QStringLiteral("7zz"), QStringLiteral("7zr")})
            t.sevenZipPath = QStandardPaths::findExecutable(alt);
    }

    t.unrarVersion = versionOf(t.unrarPath);
    t.rarVersion = versionOf(t.rarPath, QStringLiteral("-iver"));
    return t;
}

QString BinaryLocator::versionOf(const QString &binary, const QString &versionSwitch)
{
    if (binary.isEmpty())
        return {};

    QStringList args;
    if (!versionSwitch.isEmpty())
        args << versionSwitch;

    QProcess p;
    p.start(binary, args);
    if (!p.waitForFinished(4000))
        return {};
    const QString out = QString::fromUtf8(p.readAllStandardOutput());
    if (out.isEmpty())
        return {};

    // Lineas tipo:
    //   "UNRAR 7.23 freeware      Copyright (c) 1993-2026 Alexander Roshal"
    //   "7-Zip (7zz) 24.09 (x64)"
    const QString &first = out.section(QLatin1Char('\n'), 0, 0);
    static const QRegularExpression re(QStringLiteral("(\\d+\\.\\d+)"));
    const QRegularExpressionMatch m = re.match(first);
    return m.hasMatch() ? m.captured(1) : QString();
}

QStringList BinaryLocator::licenseKeySearchPaths()
{
    // Segun el README de rarlab, RAR/Unix busca rarreg.key en estos sitios.
    // QtRAR solo informa; la clave la usa el propio binario `rar`.
    return {
        QDir::homePath() + QStringLiteral("/.rarreg.key"),
        QDir::homePath() + QStringLiteral("/rarreg.key"),
        QStringLiteral("/etc/rarreg.key"),
        QStringLiteral("/usr/lib/rarreg.key"),
        QStringLiteral("/usr/local/lib/rarreg.key"),
        QStringLiteral("/usr/local/etc/rarreg.key"),
    };
}

} // namespace qtrar
