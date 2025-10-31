#pragma once
#include <QObject>
#include <QProcess>

class VenvManager : public QObject
{
    Q_OBJECT
public:
    explicit VenvManager(QObject *parent = nullptr);
    bool createVenv(const QString &dir, const QString &pythonVer);
    bool upgradePip(const QString &pipVer, const QString &pipToolsVer);
    QString venvPython() const { return m_python; }

signals:
    void logMessage(const QString &line);

private:
    QString m_venvDir;
    QString m_python;
    bool runCmd(const QStringList &cmd, const QProcessEnvironment &penv = QProcessEnvironment());
};