/****************************************************************
 * @file TerminalEngine.cpp
 * @brief Implements the TerminalEngine class for terminal operations.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.8
 * @date    2025-11-04
 * @section License MIT
 * @section DESCRIPTION
 * This file contains the implementation of TerminalEngine class.
 ***************************************************************/
#include "TerminalEngine.h"
#include <QFileInfo>
#include <QDebug>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QMessageBox>
#include <QDesktopServices>
#include <QSettings>
#include <QUrl>
#include <QPushButton>
#include <QProcess>
#include "Settings.h"        // central source of truth
#include "Config.h"

#define SHOW_DEBUG 1

// Static globals
QString TerminalEngine::g_pythonExe;
QStringList TerminalEngine::g_pythonBaseArgs;
/****************************************************************
 * @brief Constructor: Initializes the terminal engine.
 ***************************************************************/
TerminalEngine::TerminalEngine(QObject *parent) : QObject(parent), currentProcess(nullptr)
{
    venvPath = QDir::current().filePath(".venv");
}

/****************************************************************
 * @brief Destructor: Cleans up resources.
 ***************************************************************/
TerminalEngine::~TerminalEngine()
{
    if (currentProcess)
    {
        if (currentProcess->state() != QProcess::NotRunning)
        {
            currentProcess->kill();
            currentProcess->waitForFinished();
        }
        delete currentProcess;
    }
}

/****************************************************************
 * @brief Sets the virtual environment path.
 ***************************************************************/
void TerminalEngine::setVenvPath(const QString &path)
{
    venvPath = path;
}

/****************************************************************
 * @brief Gets the current virtual environment path.
 ***************************************************************/
QString TerminalEngine::getVenvPath() const
{
    return venvPath;
}

/****************************************************************
 * @brief Sets the global Python command based on Settings version.
 *        Validates actual interpreter version and warns if mismatch.
 * @param versionFromSettings Version string (e.g., "3.10 or 3.11").
 ***************************************************************/
/****************************************************************
 * @brief Sets the Python command based on Settings or defaults.
 *        Probes the actual interpreter version and handles
 *        mismatches by prompting the user to install or switch.
 *
 * @param versionFromSettings The Python version string from Settings.
 ***************************************************************/
void TerminalEngine::setPythonCommand(const QString &versionFromSettings)
{
    g_pythonExe.clear();
    g_pythonBaseArgs.clear();

#ifdef Q_OS_WIN
    QString pyPath = QStandardPaths::findExecutable("py");
    if (!pyPath.isEmpty())
    {
        g_pythonExe = pyPath;
        g_pythonBaseArgs << (versionFromSettings.isEmpty() ? "-3" : "-" + versionFromSettings);
    }
    else
    {
        QString pythonPath = QStandardPaths::findExecutable("python");
        g_pythonExe = pythonPath.isEmpty() ? "python" : pythonPath;
    }
#else
    g_pythonExe = versionFromSettings.isEmpty()
                      ? "python3"
                      : QString("python%1").arg(versionFromSettings);
#endif

    // Probe actual version
    QProcess proc;
    proc.start(g_pythonExe, g_pythonBaseArgs + QStringList{"--version"});
    proc.waitForFinished(3000);
    QString output = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();

    if (!versionFromSettings.isEmpty() && !output.contains(versionFromSettings))
    {
        // Detect available versions
        QStringList detectedVersions;
        QStringList candidates = {"python3.10", "python3.11", "python3.12", "python3.13", "python"};

        // FIX: use index-based loop to avoid detach warning
        for (int i = 0; i < candidates.size(); ++i)
        {
            const QString &candidate = candidates.at(i);
            QString exe = QStandardPaths::findExecutable(candidate);
            if (!exe.isEmpty())
            {
                QProcess p;
                p.start(exe, {"--version"});
                p.waitForFinished(2000);
                QString verOut = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
                if (!verOut.isEmpty())
                {
                    detectedVersions << verOut;
                }
            }
        }

        // Load configured default version from Settings
        QSettings settings;
        QString defaultVersion = settings.value("PythonVersion", "3.10").toString();

        QMessageBox msgBox;
        msgBox.setWindowTitle(tr("Python Version Mismatch"));
        msgBox.setText(tr("Requested Python %1, but found %2.")
                           .arg(versionFromSettings, output));
        msgBox.setInformativeText(tr("Choose an action:"));

        QPushButton *installBtn = msgBox.addButton(
            tr("Install Python %1").arg(defaultVersion),
            QMessageBox::ActionRole);

        QVector<QPushButton*> switchButtons;
        for (int i = 0; i < detectedVersions.size(); ++i)
        {
            const QString &ver = detectedVersions.at(i);
            QPushButton *btn = msgBox.addButton(
                tr("Switch default to %1").arg(ver),
                QMessageBox::ActionRole);
            switchButtons.append(btn);
        }

        msgBox.addButton(QMessageBox::Cancel);
        msgBox.exec();

        if (msgBox.clickedButton() == installBtn)
        {
            // Build installer URL dynamically
            QString installerUrl = QString("https://www.python.org/ftp/python/%1.0/python-%1.0-amd64.exe")
                                       .arg(defaultVersion);
            QDesktopServices::openUrl(QUrl(installerUrl));

            // Update Settings to reflect chosen default
            settings.setValue("PythonVersion", defaultVersion);
            settings.sync();
        }
        else
        {
            for (int i = 0; i < switchButtons.size(); ++i)
            {
                if (msgBox.clickedButton() == switchButtons[i])
                {
                    QString chosenVersion = detectedVersions[i].section(' ', 1, 1);
                    settings.setValue("PythonVersion", chosenVersion);
                    settings.sync();

                    // Re‑apply with new version
                    TerminalEngine::setPythonCommand(chosenVersion);
                    break;
                }
            }
        }
    }

    DEBUG_MSG() << "[DEBUG] setPythonCommand resolved:" << g_pythonExe << g_pythonBaseArgs << "Reported:" << output;
}

/****************************************************************
 * @brief Returns base args (e.g., -3.11).
 ***************************************************************/
QStringList TerminalEngine::pythonBaseArgs()
{
    return g_pythonBaseArgs;
}

/****************************************************************
 * @brief Creates a Python virtual environment and upgrades pip/pip-tools.
 *        Handles both Windows launcher (py.exe) and direct python.exe.
 *        Removes any existing venv first, then runs the command.
 *
 * @param pythonVersion Version selector (e.g. "-3.10") if using py.exe
 * @return true if venv created successfully, false otherwise
 ***************************************************************/
bool TerminalEngine::createVirtualEnvironment(const QString &pythonVersion)
{
    DEBUG_MSG() << "Enter createVirtualEnvironment()";
    DEBUG_MSG() << "Target venv path:" << venvPath;

    emit venvProgress("Checking for existing virtual environment...");

    if (venvExists())
    {
        DEBUG_MSG() << "Existing venv found, removing...";
        if (!removeVirtualEnvironment())
        {
            emit outputReceived("Failed to remove existing virtual environment", true);
            return false;
        }
    }

    QString pythonExe = pythonCommand();
    QStringList args = pythonBaseArgs();

    if (pythonExe.contains("py.exe", Qt::CaseInsensitive) && !pythonVersion.isEmpty())
    {
        DEBUG_MSG() << "Detected py.exe → adding version selector:" << pythonVersion;
        args << pythonVersion << "-m" << "venv" << venvPath;
    }
    else
    {
        DEBUG_MSG() << "Detected direct python interpreter → no version selector";
        args << "-m" << "venv" << venvPath;
    }

    DEBUG_MSG() << "Using Python executable:" << pythonExe;
    DEBUG_MSG() << "Command line:" << pythonExe << args;

    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(pythonExe, args);

    if (!process.waitForStarted(5000))
    {
        emit outputReceived("Failed to start Python process", true);
        return false;
    }

    bool finished = process.waitForFinished(60000);

    QString stdOut = QString::fromUtf8(process.readAllStandardOutput());
    QString stdErr = QString::fromUtf8(process.readAllStandardError());

    DEBUG_MSG() << "Process finished:" << finished
                << " Exit code:" << process.exitCode()
                << " Exit status:" << process.exitStatus()
                << " Error:" << process.error();
    DEBUG_MSG() << "StdOut:" << stdOut;
    DEBUG_MSG() << "StdErr:" << stdErr;

    if (!finished || process.exitCode() != 0 || !venvExists())
    {
        emit outputReceived(QString("Failed to create virtual environment: %1").arg(stdErr), true);
        return false;
    }

    emit venvProgress("Virtual environment created successfully");

    // 🔧 NEW STEP: upgrade pip and install pip-tools
    QString venvPython = venvPath + "/Scripts/python.exe"; // Windows path
    QStringList upgradeArgs;
    upgradeArgs << "-m" << "pip" << "install" << "--upgrade" << "pip" << "pip-tools";

    DEBUG_MSG() << "Upgrading pip and installing pip-tools...";
    QProcess upgradeProc;
    upgradeProc.setProcessChannelMode(QProcess::MergedChannels);
    upgradeProc.start(venvPython, upgradeArgs);
    upgradeProc.waitForFinished(60000);

    QString upgradeOut = QString::fromUtf8(upgradeProc.readAllStandardOutput());
    QString upgradeErr = QString::fromUtf8(upgradeProc.readAllStandardError());

    DEBUG_MSG() << "Upgrade StdOut:" << upgradeOut;
    DEBUG_MSG() << "Upgrade StdErr:" << upgradeErr;

    if (upgradeProc.exitCode() != 0)
    {
        emit outputReceived(QString("pip upgrade failed: %1").arg(upgradeErr), true);
        return false;
    }

    emit venvProgress("pip and pip-tools upgraded successfully");
    return true;
}

/****************************************************************
 * @brief Attempts to activate the virtual environment by probing python.
 * @return true if activation succeeds, false otherwise.
 ***************************************************************/
bool TerminalEngine::activateVenv()
{
    DEBUG_MSG() << "Enter activateVenv()";

    // Use global Python command resolved at startup
    QString pythonExe = TerminalEngine::pythonCommand();
    QStringList args = TerminalEngine::pythonBaseArgs();
    args << "--version";

    DEBUG_MSG() << "Activation probe:" << pythonExe << args;

    QProcess proc;
    proc.start(pythonExe, args);

    if (!proc.waitForStarted(5000))
    {
        DEBUG_MSG() << "Activation probe failed to start:" << proc.errorString();
        return false;
    }

    bool finished = proc.waitForFinished(10000);
    QString stdOut = QString::fromUtf8(proc.readAllStandardOutput());
    QString stdErr = QString::fromUtf8(proc.readAllStandardError());

    DEBUG_MSG() << "Finished:" << finished
                          << "Exit code:" << proc.exitCode()
                          << "StdOut:" << stdOut
                          << "StdErr:" << stdErr;

    if (!finished || proc.exitCode() != 0)
    {
        return false;
    }

    // If we got a version string back, consider activation successful
    return !stdOut.isEmpty() || !stdErr.isEmpty();
}


/****************************************************************
 * @brief Gets venv status information.
 * @return Status string describing venv state.
 ***************************************************************/
QString TerminalEngine::getVenvStatus() const
{
    if (!venvExists())
    {
        return QString("venv: missing (%1)").arg(venvPath);
    }

    QFileInfo pyInfo(getPythonExecutable());
    QFileInfo pipInfo(getPipExecutable());

    QStringList parts;
    parts << QString("venv: present (%1)").arg(venvPath);
    parts << QString("python: %1 %2").arg(pyInfo.exists() ? "OK" : "MISSING").arg(pyInfo.filePath());
    parts << QString("pip: %1 %2").arg(pipInfo.exists() ? "OK" : "MISSING").arg(pipInfo.filePath());

    return parts.join(" | ");
}

/****************************************************************
 * @brief Checks if virtual environment exists.
 ***************************************************************/
bool TerminalEngine::venvExists() const
{
    QFileInfo venvInfo(venvPath);
    if (!venvInfo.exists() || !venvInfo.isDir())
    {
        return false;
    }

    // Check for Python executable
    QString pythonExe = getPythonExecutable();
    QFileInfo pythonInfo(pythonExe);
    return pythonInfo.exists() && pythonInfo.isFile();
}

/****************************************************************
 * @brief Upgrades pip inside the virtual environment.
 * @return true if successful, false otherwise.
 ***************************************************************/
bool TerminalEngine::upgradePip()
{
    QStringList args = TerminalEngine::pythonBaseArgs();
    args << "-m" << "pip" << "install" << "--upgrade" << "pip";
    QString pythonExe = TerminalEngine::pythonCommand();

    DEBUG_MSG() << "Upgrading pip with:" << pythonExe << args;

    QProcess proc;
    proc.start(pythonExe, args);
    proc.waitForFinished(30000);

    DEBUG_MSG() << "Exit code:" << proc.exitCode() << "StdErr:" << QString::fromUtf8(proc.readAllStandardError());

    return proc.exitCode() == 0;
}

/****************************************************************
 * @brief Installs pip-tools in the virtual environment.
 ***************************************************************/
bool TerminalEngine::installPipTools(const QString &version)
{
    if (!venvExists())
    {
        emit outputReceived("Virtual environment does not exist", true);
        return false;
    }

    QString pythonExe = getPythonExecutable();
    QStringList args;
    args << "-m" << "pip" << "install";

    if (version.isEmpty())
    {
        args << "pip-tools";
    }
    else
    {
        args << QString("pip-tools==%1").arg(version);
    }

    QProcess process;
    process.start(pythonExe, args);
    process.waitForFinished(120000); // 2 minute timeout

    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    emit outputReceived(output, false);
    if (!error.isEmpty())
    {
        emit outputReceived(error, true);
    }

    return process.exitCode() == 0;
}

/****************************************************************
 * @brief Executes a command in the terminal.
 ***************************************************************/
void TerminalEngine::executeCommand(const QString &command)
{
    QString cmd = command.trimmed();
    if (cmd.isEmpty())
    {
        emit outputReceived("No command entered", true);
        return;
    }

    currentCommand = cmd;
    emit commandStarted(currentCommand);

    logMessage(QString("$ %1").arg(currentCommand));

    parseAndExecuteCommand(currentCommand);
}

/****************************************************************
 * @brief Stops the currently running process.
 ***************************************************************/
void TerminalEngine::stopCurrentProcess()
{
    if (currentProcess && currentProcess->state() != QProcess::NotRunning)
    {
        emit outputReceived("Terminating process...", false);
        currentProcess->kill();
        currentProcess->waitForFinished();
        emit outputReceived("Process terminated", false);
    }
}

/****************************************************************
 * @brief Gets the Python executable path for the venv.
 ***************************************************************/
QString TerminalEngine::getPythonExecutable() const
{
#if defined(Q_OS_WIN)
    return QDir(venvPath).filePath("Scripts/python.exe");
#else
    return QDir(venvPath).filePath("bin/python");
#endif
}

/****************************************************************
 * @brief Gets the pip executable path for the venv.
 ***************************************************************/
QString TerminalEngine::getPipExecutable() const
{
#if defined(Q_OS_WIN)
    return QDir(venvPath).filePath("Scripts/pip.exe");
#else
    return QDir(venvPath).filePath("bin/pip");
#endif
}

/****************************************************************
 * @brief Slot for standard output ready.
 ***************************************************************/
void TerminalEngine::onReadyReadStandardOutput()
{
    if (currentProcess)
    {
        QString output = QString::fromUtf8(currentProcess->readAllStandardOutput());
        emit outputReceived(output, false);
    }
}

/****************************************************************
 * @brief Slot for standard error ready.
 ***************************************************************/
void TerminalEngine::onReadyReadStandardError()
{
    if (currentProcess)
    {
        QString error = QString::fromUtf8(currentProcess->readAllStandardError());
        emit outputReceived(error, true);
    }
}

/****************************************************************
 * @brief Slot for process finished.
 ***************************************************************/
void TerminalEngine::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit)
    {
        emit outputReceived("Process crashed", true);
    }
    else if (exitCode != 0)
    {
        emit outputReceived(QString("Process exited with code %1").arg(exitCode), true);
    }

    emit commandFinished(exitCode, exitStatus);
}

/****************************************************************
 * @brief Slot for process error.
 ***************************************************************/
void TerminalEngine::onProcessError(QProcess::ProcessError error)
{
    QString errorMsg;
    switch (error)
    {
    case QProcess::FailedToStart:
        errorMsg = "Failed to start process";
        break;
    case QProcess::Crashed:
        errorMsg = "Process crashed";
        break;
    case QProcess::Timedout:
        errorMsg = "Process timed out";
        break;
    case QProcess::WriteError:
        errorMsg = "Write error";
        break;
    case QProcess::ReadError:
        errorMsg = "Read error";
        break;
    default:
        errorMsg = "Unknown error";
        break;
    }

    emit outputReceived(errorMsg, true);
}

/****************************************************************
 * @brief Parses and routes the command to appropriate handler.
 ***************************************************************/
void TerminalEngine::parseAndExecuteCommand(const QString &command)
{
    // Make a copy to ensure we have a stable string
    QString cmd = command;

    // Check for pip commands
    if (cmd.startsWith("pip "))
    {
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        parts.removeFirst(); // Remove "pip"
        executePipCommand(parts);
    }
    // Check for pip-compile
    else if (cmd.startsWith("pip-compile"))
    {
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        parts.removeFirst(); // Remove "pip-compile"
        executePipToolsCommand("compile", parts);
    }
    // Check for pip-sync
    else if (cmd.startsWith("pip-sync"))
    {
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        parts.removeFirst(); // Remove "pip-sync"
        executePipToolsCommand("sync", parts);
    }
    // Check for python commands
    else if (cmd.startsWith("python "))
    {
        QStringList parts = cmd.split(' ', Qt::SkipEmptyParts);
        parts.removeFirst(); // Remove "python"
        executePythonScript(parts);
    }
    // Check for venv activation (informational only)
    else if (cmd == "activate" || cmd.contains("activate"))
    {
        emit outputReceived("Note: Virtual environment is automatically active in this terminal", false);
        emit outputReceived(QString("Using venv: %1").arg(venvPath), false);
        emit commandFinished(0, QProcess::NormalExit);
    }
    // Check for deactivate (informational only)
    else if (cmd == "deactivate")
    {
        emit outputReceived("Note: Virtual environment cannot be deactivated in this terminal", false);
        emit commandFinished(0, QProcess::NormalExit);
    }
    // Shell commands
    else
    {
        executeShellCommand(cmd);
    }
}

/****************************************************************
 * @brief Executes a pip command.
 ***************************************************************/
void TerminalEngine::executePipCommand(const QStringList &args)
{
    if (!venvExists())
    {
        emit outputReceived("Virtual environment does not exist. Create one first.", true);
        emit commandFinished(1, QProcess::NormalExit);
        return;
    }

    if (currentProcess)
    {
        delete currentProcess;
    }

    currentProcess = new QProcess(this);
    connect(currentProcess, &QProcess::readyReadStandardOutput, this, &TerminalEngine::onReadyReadStandardOutput);
    connect(currentProcess, &QProcess::readyReadStandardError, this, &TerminalEngine::onReadyReadStandardError);
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalEngine::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred, this, &TerminalEngine::onProcessError);

    QString pythonExe = getPythonExecutable();
    QStringList fullArgs;
    fullArgs << "-m" << "pip" << args;

    currentProcess->start(pythonExe, fullArgs);
}

/****************************************************************
 * @brief Executes a pip-tools command.
 ***************************************************************/
void TerminalEngine::executePipToolsCommand(const QString &tool, const QStringList &args)
{
    if (!venvExists())
    {
        emit outputReceived("Virtual environment does not exist. Create one first.", true);
        emit commandFinished(1, QProcess::NormalExit);
        return;
    }

    if (currentProcess)
    {
        delete currentProcess;
    }

    currentProcess = new QProcess(this);
    connect(currentProcess, &QProcess::readyReadStandardOutput, this, &TerminalEngine::onReadyReadStandardOutput);
    connect(currentProcess, &QProcess::readyReadStandardError, this, &TerminalEngine::onReadyReadStandardError);
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalEngine::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred, this, &TerminalEngine::onProcessError);

    QString pythonExe = getPythonExecutable();
    QStringList fullArgs;
    fullArgs << "-m" << "piptools" << tool << args;

    currentProcess->start(pythonExe, fullArgs);
}

/****************************************************************
 * @brief Executes a Python script.
 ***************************************************************/
void TerminalEngine::executePythonScript(const QStringList &args)
{
    if (!venvExists())
    {
        emit outputReceived("Virtual environment does not exist. Create one first.", true);
        emit commandFinished(1, QProcess::NormalExit);
        return;
    }

    if (currentProcess)
    {
        delete currentProcess;
    }

    currentProcess = new QProcess(this);
    connect(currentProcess, &QProcess::readyReadStandardOutput, this, &TerminalEngine::onReadyReadStandardOutput);
    connect(currentProcess, &QProcess::readyReadStandardError, this, &TerminalEngine::onReadyReadStandardError);
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalEngine::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred, this, &TerminalEngine::onProcessError);

    QString pythonExe = getPythonExecutable();
    currentProcess->start(pythonExe, args);
}

/****************************************************************
 * @brief Executes a shell command.
 ***************************************************************/
void TerminalEngine::executeShellCommand(const QString &command)
{
    if (currentProcess)
    {
        delete currentProcess;
    }

    currentProcess = new QProcess(this);
    connect(currentProcess, &QProcess::readyReadStandardOutput, this, &TerminalEngine::onReadyReadStandardOutput);
    connect(currentProcess, &QProcess::readyReadStandardError, this, &TerminalEngine::onReadyReadStandardError);
    connect(currentProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalEngine::onProcessFinished);
    connect(currentProcess, &QProcess::errorOccurred, this, &TerminalEngine::onProcessError);

    QString shell = getShell();
    QStringList shellArgs = getShellArgs(command);

    currentProcess->start(shell, shellArgs);
}

/****************************************************************
 * @brief Removes existing virtual environment.
 ***************************************************************/
bool TerminalEngine::removeVirtualEnvironment()
{
    if (!venvExists())
    {
        return true; // Already doesn't exist
    }

    QDir venvDir(venvPath);
    return venvDir.removeRecursively();
}

/****************************************************************
 * @brief Gets the platform-specific shell.
 ***************************************************************/
QString TerminalEngine::getShell() const
{
#if defined(Q_OS_WIN)
    return "cmd.exe";
#else
    return "bash";
#endif
}

/****************************************************************
 * @brief Gets shell arguments for command execution.
 ***************************************************************/
QStringList TerminalEngine::getShellArgs(const QString &command) const
{
    QStringList args;
#if defined(Q_OS_WIN)
    args << "/C" << command;
#else
    args << "-c" << command;
#endif
    return args;
}

/****************************************************************
 * @brief Logs a message with timestamp.
 ***************************************************************/
void TerminalEngine::logMessage(const QString &message, bool isError)
{
    QString timestamp = QDateTime::currentDateTime().toString("[HH:mm:ss]");
    emit outputReceived(QString("%1 %2").arg(timestamp, message), isError);
}

bool TerminalEngine::isCommandRunnable(const QString& command) const
{
    QProcess proc;
    QStringList args;
    args << QStringLiteral("--version");

    proc.start(command, args);
    bool started = proc.waitForStarted(3000);
    if (!started)
    {
        return false;
    }

    bool finished = proc.waitForFinished(5000);
    if (!finished)
    {
        proc.kill();
        proc.waitForFinished(2000);
        return false;
    }

    int exitCode = proc.exitCode();
    return (exitCode == 0);
}

QString TerminalEngine::pythonCommand() const
{
    const QString configured = Settings::instance()->pythonInterpreter();

    if (isCommandRunnable(configured))
    {
        return configured;
    }

    qWarning() << "Configured Python interpreter not runnable:" << configured;

    const QString fallback = Settings::instance()->defaultPythonInterpreter();
    if (isCommandRunnable(fallback))
    {
        qInfo() << "Falling back to default Python interpreter:" << fallback;
        return fallback;
    }

    qCritical() << "No valid Python interpreter found. Update Settings or install the required version.";
    return configured;
}

/****************************************************************
 * @brief Returns the path to the Python executable inside a venv.
 *        Handles Windows, Linux, and macOS layouts.
 ***************************************************************/
QString TerminalEngine::venvPythonPath(const QString& venvPath) const
{
#ifdef Q_OS_WIN
    return QDir(venvPath).filePath("Scripts/python.exe");
#else
    return QDir(venvPath).filePath("bin/python3");
#endif
}

/************** End of TerminalEngine.cpp ***********************/
