#pragma once
#include <QObject>
#include <QQueue>
#include <QProcess>

struct BatchJob
{
    QString imagePath;
    QString audioPath;
    QString outputPath;
};

class BatchRunner : public QObject
{
    Q_OBJECT
public:
    explicit BatchRunner(QObject *parent = nullptr);

    void enqueue(const BatchJob &job);
    void start();
    void stop();

signals:
    void logMessage(const QString &line);
    void progressChanged(int percent);
    void jobFinished(const QString &outputPath);
    void allJobsFinished();

private slots:
    void handleReadyRead();
    void handleFinished(int exitCode, QProcess::ExitStatus status);

private:
    void runNextJob();
    int parseProgress(const QString &line);

    QQueue<BatchJob> m_jobs;
    QProcess m_process;
    BatchJob m_currentJob;
    bool m_running;
};