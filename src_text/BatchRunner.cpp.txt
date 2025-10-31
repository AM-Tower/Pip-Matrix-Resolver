#include "BatchRunner.h"
#include <QRegularExpression>
#include <QFileInfo>

BatchRunner::BatchRunner(QObject *parent) : QObject(parent), m_running(false)
{
    connect(&m_process, &QProcess::readyReadStandardError, this, &BatchRunner::handleReadyRead);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &BatchRunner::handleFinished);
}

void BatchRunner::enqueue(const BatchJob &job)
{
    m_jobs.enqueue(job);
}

void BatchRunner::start()
{
    if (m_running || m_jobs.isEmpty())
    {
        return;
    }
    m_running = true;
    runNextJob();
}

void BatchRunner::stop()
{
    if (m_running)
    {
        m_process.kill();
        m_jobs.clear();
        m_running = false;
    }
}

void BatchRunner::runNextJob()
{
    if (m_jobs.isEmpty())
    {
        m_running = false;
        emit allJobsFinished();
        return;
    }

    m_currentJob = m_jobs.dequeue();
    emit logMessage(QString("Starting batch job: %1 + %2 → %3")
                    .arg(m_currentJob.imagePath, m_currentJob.audioPath, m_currentJob.outputPath));

    QStringList args = {
        "-y",
        "-i", m_currentJob.audioPath,
        "-i", m_currentJob.imagePath,
        "-c:v", "libx264",
        "-c:a", "aac",
        "-shortest",
        m_currentJob.outputPath
    };

    m_process.setProgram("ffmpeg");
    m_process.setArguments(args);
    m_process.start();
}

void BatchRunner::handleReadyRead()
{
    const QString output = QString::fromUtf8(m_process.readAllStandardError());
    emit logMessage(output);

    // Parse ffmpeg progress lines like: "frame=  100 fps=25 q=28.0 size=..."
    const QStringList lines = output.split('\n');
    for (const QString &line : lines)
    {
        int percent = parseProgress(line);
        if (percent >= 0)
        {
            emit progressChanged(percent);
        }
    }
}

void BatchRunner::handleFinished(int exitCode, QProcess::ExitStatus status)
{
    if (status == QProcess::NormalExit && exitCode == 0)
    {
        emit logMessage(QString("Job finished successfully: %1").arg(m_currentJob.outputPath));
        emit jobFinished(m_currentJob.outputPath);
    }
    else
    {
        emit logMessage(QString("Job failed: %1").arg(m_currentJob.outputPath));
    }

    runNextJob();
}

int BatchRunner::parseProgress(const QString &line)
{
    // Very basic parser: look for "time=" and estimate percent
    QRegularExpression re("time=(\\d+):(\\d+):(\\d+\\.\\d+)");
    QRegularExpressionMatch match = re.match(line);
    if (match.hasMatch())
    {
        int hours = match.captured(1).toInt();
        int minutes = match.captured(2).toInt();
        double seconds = match.captured(3).toDouble();
        double elapsed = hours * 3600 + minutes * 60 + seconds;

        // For demo: assume 60 seconds total
        double percent = (elapsed / 60.0) * 100.0;
        if (percent > 100.0) percent = 100.0;
        return static_cast<int>(percent);
    }
    return -1;
}