#include "VenvManager.h"
#include <QDir>
#include <QFileInfo>

VenvManager::VenvManager(QObject *parent) : QObject(parent)
{
}

bool VenvManager::runCmd(const QStringList &cmd, const QProcessEnvironment &penv)
{
    QProcess p;
    if (!penv.isEmpty()) p.setProcessEnvironment(penv);
    p.setProgram(cmd.first());
    p.setArguments(cmd.mid(1));
    connect(&p, &QProcess::readyReadStandardOutput, [&]()
    {
        emit logMessage(QString::fromUtf8(p.readAllStandardOutput()));
    });
    connect(&p, &QProcess::readyReadStandardError, [&]()
    {
        emit logMessage(QString::fromUtf8(p.readAllStandardError()));
    });
    p.start();
    if (!p.waitForStarted()) return false;
    p.waitForFinished(-1);
    return p.exitStatus() == QProcess::NormalExit && p.exitCode() == 0;
}

bool VenvManager::createVenv(const QString &dir, const QString &pythonVer)
{
    m_venvDir = dir;
    QDir().mkpath(dir);
    QString py = QString("python%1").arg(pythonVer);
    if (!runCmd({py, "-m", "venv", dir})) return false;
#ifdef Q_OS_WIN
    m_python = QDir(dir).filePath("Scripts/python.exe");
#else
    m_python = QDir(dir).filePath("bin/python");
#endif
    return QFileInfo::exists(m_python);
}

bool VenvManager::upgradePip(const QString &pipVer, const QString &pipToolsVer)
{
    return runCmd({m_python, "-m", "pip", "install", "--upgrade", QString("pip==%1").arg(pipVer), "setuptools", "wheel"})
        && runCmd({m_python, "-m", "pip", "install", "--upgrade", QString("pip-tools==%1").arg(pipToolsVer)});
}