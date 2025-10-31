#pragma once
#include <QObject>

class PipToolsRunner : public QObject
{
    Q_OBJECT
public:
    explicit PipToolsRunner(const QString &python, QObject *parent = nullptr);
    bool pipCompile(const QString &inFile, const QString &outFile, int retries);

signals:
    void logMessage(const QString &line);

private:
    QString m_python;
    bool runOnce(const QString &inFile, const QString &outFile);
    void analyzeLog(const QString &logPath);
};