#include "PipToolsRunner.h"
#include <QProcess>
#include <QFile>
#include <QThread>

PipToolsRunner::PipToolsRunner(const QString &python, QObject *parent) : QObject(parent), m_python(python)
{
}

bool PipToolsRunner::runOnce(const QString &inFile, const QString &outFile)
{
    QProcess p;
    p.setProgram("pip-compile");
    p.setArguments({
        "--resolver=backtracking",
        "--prefer-binary",
        "--upgrade",
        "--output-file", outFile,
        inFile
    });

    connect(&p, &QProcess::readyReadStandardOutput, [&]()
    {
        emit logMessage(QString::fromUtf8(p.readAllStandardOutput()));
    });
    connect(&p, &QProcess::readyReadStandardError, [&]()
    {
        emit logMessage(QString::fromUtf8(p.readAllStandardError()));
    });

    p.start();
    if (!p.waitForStarted())
    {
        emit logMessage("Failed to start pip-compile process.");
        return false;
    }

    p.waitForFinished(-1);
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool PipToolsRunner::pipCompile(const QString &inFile, const QString &outFile, int retries)
{
    for (int attempt = 1; attempt <= retries; ++attempt)
    {
        emit logMessage(QString("Attempting pip-compile (try %1/%2)").arg(attempt).arg(retries));
        if (runOnce(inFile, outFile))
        {
            emit logMessage("pip-compile succeeded.");
            return true;
        }

        analyzeLog("logs/pip_compile.log");
        if (attempt < retries)
        {
            emit logMessage("Retrying in 5 seconds...");
            QThread::sleep(5);
        }
    }
    return false;
}

void PipToolsRunner::analyzeLog(const QString &logPath)
{
    QFile f(logPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        emit logMessage("No pip-compile log found to analyze.");
        return;
    }

    const QString text = QString::fromUtf8(f.readAll());
    if (text.contains("No matching distribution found for"))
    {
        emit logMessage("Detected missing distribution; consider adjusting baseline.");
    }
    else if (text.contains("requires"))
    {
        emit logMessage("Dependency conflict detected; consider adjusting dependent baseline.");
    }
}