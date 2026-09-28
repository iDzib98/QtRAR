#include "ProcessRunner.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QRegularExpression>
#include <QStringList>

namespace qtrar {

ProcessRunner::ProcessRunner(QObject *parent) : QObject(parent) {}

ProcessRunner::~ProcessRunner()
{
    if (m_process.state() != QProcess::NotRunning) {
        m_process.kill();
        m_process.waitForFinished(2000);
    }
}

QString ProcessRunner::passwordSwitch(bool hasPassword, const QString &password)
{
    // Clave vacia o sin clave -> "-p-" (consultar el manual de rar.txt):
    // con -p- unrar no pregunta interactivamente y falla con codigo 11.
    if (!hasPassword || password.isEmpty())
        return QStringLiteral("-p-");
    return QStringLiteral("-p") + password;
}

bool ProcessRunner::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

bool ProcessRunner::cancel()
{
    if (m_process.state() == QProcess::NotRunning)
        return false;
    m_process.terminate();
    if (!m_process.waitForFinished(1500)) {
        m_process.kill();
        m_process.waitForFinished(1500);
    }
    return true;
}

RunResult ProcessRunner::run(const QString &binary, const QStringList &args,
                             const QString &stdinData, int timeoutMs)
{
    RunResult r;
    if (binary.isEmpty()) {
        r.exitCode = -1;
        return r;
    }

    m_process.setProgram(binary);
    m_process.setArguments(args);
    // Unrar escribe los mensajes en stdout y algunos errores en stderr.
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    m_process.start();
    if (!m_process.waitForStarted(5000)) {
        r.exitCode = -1;
        r.started = false;
        return r;
    }
    r.started = true;

    // Defensa 2: cualquier peticion de entrada recibe EOF.
    if (stdinData.isNull()) {
        m_process.closeWriteChannel();
    } else {
        m_process.write(stdinData.toUtf8());
        m_process.closeWriteChannel();
    }

    QElapsedTimer timer;
    timer.start();

    QString out, err;
    QByteArray pending;

    while (true) {
        if (m_process.waitForReadyRead(100)) {
            out += QString::fromUtf8(m_process.readAllStandardOutput());
            err += QString::fromUtf8(m_process.readAllStandardError());
        }
        if (m_process.state() == QProcess::NotRunning) {
            out += QString::fromUtf8(m_process.readAllStandardOutput());
            err += QString::fromUtf8(m_process.readAllStandardError());
            break;
        }
        if (timeoutMs > 0 && timer.elapsed() > timeoutMs) {
            r.timedOut = true;
            cancel();
            break;
        }
    }

    r.exitCode = m_process.exitCode();
    r.cancelled = (r.exitCode == static_cast<int>(ExitCode::UserBreak));
    r.stdOut = out;
    r.stdErr = err;
    r.stdIn = stdinData;
    return r;
}

bool parseProgressLine(const QString &line, QString *message, int *percent)
{
    // Formatos reales observados:
    //   "Adding    ./readme.txt                                   14% OK "
    //   "Extracting  docs/manual.txt                            100% OK "
    //   "Testing    readme.txt                                  OK "
    //   "Deleting from x.rar"                                   (no es progreso)
    static const QRegularExpression re(
        QStringLiteral("^(Adding|Extracting|Testing|Deleting|Updating|Compressing)\\s+"
                       "(.+?)\\s+(\\d{1,3})%"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = re.match(line);
    if (!m.hasMatch())
        return false;

    if (message)
        *message = m.captured(2).trimmed();
    if (percent)
        *percent = m.captured(3).toInt();
    return true;
}

QString extractErrorMessage(const QString &stdErr, const QString &stdOut)
{
    // Mensajes que hay que descartar por ser ruido del trialware o del banner.
    const QStringList ignore = {
        QStringLiteral("Evaluation copy"),
        QStringLiteral("Please register"),
        QStringLiteral("freeware"),
        QStringLiteral("Copyright (c)"),
        QStringLiteral("UNREGISTERED"),
        QStringLiteral("Trial version"),
    };

    QString best;
    const auto scan = [&ignore, &best](const QString &text) {
        const QStringList lines = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        for (const QString &raw : lines) {
            const QString line = raw.trimmed();
            if (line.isEmpty())
                continue;
            bool skip = false;
            for (const QString &pat : ignore) {
                if (line.contains(pat, Qt::CaseInsensitive)) {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;
            if (best.isEmpty())
                best = line;
        }
    };
    scan(stdErr);
    if (best.isEmpty())
        scan(stdOut);
    return best;
}

} // namespace qtrar
