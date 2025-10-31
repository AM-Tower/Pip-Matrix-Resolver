#include "ResolverEngine.h"
#include "PipToolsRunner.h"

#include <QFile>
#include <QTextStream>
#include <QThread>
#include <QtMath>   // for qRound

ResolverEngine::ResolverEngine(QObject *parent)
    : QObject(parent),
    m_running(false),
    m_paused(false),
    m_stateFile("logs/ITERATION_STATE.txt"),
    valid(true)
{
}

void ResolverEngine::loadRequirementsFromFile(const QString &path)
{
    emit logMessage(QString("Loading requirements from file: %1").arg(path));
    // TODO: parse requirements.txt into m_pkgs
}

void ResolverEngine::loadRequirementsFromUrl(const QString &url)
{
    emit logMessage(QString("Fetching requirements from URL: %1").arg(url));
    // TODO: download and parse into m_pkgs
}

void ResolverEngine::start()
{
    m_running = true;
    m_paused = false;

    int combinationCount = 0;
    QFile sf(m_stateFile);
    if (sf.open(QIODevice::ReadOnly))
    {
        QTextStream ts(&sf);
        ts >> combinationCount;
        sf.close();
    }

    while (m_running)
    {
        if (m_paused)
        {
            QThread::msleep(200);
            continue;
        }

        ++combinationCount;
        QFile sfw(m_stateFile);
        if (sfw.open(QIODevice::WriteOnly | QIODevice::Truncate))
        {
            QTextStream ts(&sfw);
            ts << combinationCount;
        }

        QString inFile, comboStr;
        buildNextConstraints(inFile, comboStr);
        emit logMessage(QString("Attempt #%1: %2").arg(combinationCount).arg(comboStr));

        QString outFile = QString("logs/tmp/compiled_requirements_%1.txt").arg(combinationCount);
        PipToolsRunner runner(QString(), this);
        if (runner.pipCompile(inFile, outFile, 3))
        {
            emit successCompiled(outFile);
            break;
        }

        if (!incrementOdometer())
        {
            emit logMessage("All combinations exhausted.");
            stop();
        }

        emit progressChanged(qRound((double)combinationCount / 1000.0 * 100.0));
    }
}

void ResolverEngine::pause()
{
    m_paused = true;
}

void ResolverEngine::resume()
{
    m_paused = false;
}

void ResolverEngine::stop()
{
    m_running = false;
}

void ResolverEngine::buildNextConstraints(QString &inFile, QString &comboStr)
{
    // TODO: build constraints.in file from current indices
    inFile = "logs/tmp/temp_constraints.in";
    comboStr = "example==1.0.0";
}

bool ResolverEngine::incrementOdometer()
{
    for (int j = m_indices.size() - 1; j >= 0; --j)
    {
        m_indices[j]++;
        if (m_indices[j] <= m_maxIndices[j])
        {
            return true;
        }
        m_indices[j] = 0;
    }
    return false;
}

bool ResolverEngine::isValid() const
{
    return valid;
}

bool ResolverEngine::resolve(const QString &path)
{
    // Stub implementation: succeed if path is non-empty
    return !path.isEmpty();
}
