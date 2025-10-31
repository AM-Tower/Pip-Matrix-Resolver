#pragma once

#include <QObject>
#include <QVector>
#include <QStringList>
#include <QString>

struct PackageCandidates
{
    QString pkg;
    QStringList versions;
};

class ResolverEngine : public QObject
{
    Q_OBJECT
public:
    explicit ResolverEngine(QObject *parent = nullptr);

    void loadRequirementsFromFile(const QString &path);
    void loadRequirementsFromUrl(const QString &url);

    void start();
    void pause();
    void resume();
    void stop();

    // Example methods for unit tests
    bool isValid() const;
    bool resolve(const QString &path);

signals:
    void logMessage(const QString &line);
    void progressChanged(int percent);
    void successCompiled(const QString &compiledPath);

private:
    QVector<PackageCandidates> m_pkgs;
    QVector<int> m_indices;
    QVector<int> m_maxIndices;
    bool m_running;
    bool m_paused;
    QString m_stateFile;
    bool valid;

    void buildNextConstraints(QString &inFile, QString &comboStr);
    bool incrementOdometer();
};
