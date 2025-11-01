#pragma once
#include <QObject>
#include <QString>

class VenvManager : public QObject
{
    Q_OBJECT
public:
    explicit VenvManager(QObject *parent = nullptr);

    // Existing API
    bool createVenv(const QString &dir, const QString &pythonVer);
    bool upgradePip(const QString &pipVer, const QString &pipToolsVer);

    // New convenience wrapper
    bool createOrUpdate(const QString &dir = "venv",
                        const QString &pythonVer = "3.11",
                        const QString &pipVer = "24.0",
                        const QString &pipToolsVer = "7.4.1");

signals:
    void logMessage(const QString &line);
};
