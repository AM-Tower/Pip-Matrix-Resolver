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

/****************************************************************
 * @brief Constructor: Initializes the terminal engine.
 ***************************************************************/
TerminalEngine::TerminalEngine(QObject *parent)
    : QObject(parent),
    currentProcess(nullptr)
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
 * @brief Creates a new virtual environment.
 ***************************************************************/
bool TerminalEngine::createVirtualEnvironment(const QString &pythonVersion)
{
    emit venvProgress("Checking for existing virtual environment...");

    // Remove existing venv if it exists
    if (venvExists())
    {
        emit venvProgress("Removing existing virtual environment...");
        if (!removeVirtualEnvironment())
        {
            emit outputReceived("Failed to remove existing virtual environment", true);
            return false;
        }
    }

    emit venvProgress("Creating new virtual environment...");

    // Find Python executable
    QString pythonExe = "python";
    if (!pythonVersion.isEmpty())
    {
        pythonExe = QString("python%1").arg(pythonVersion);
    }

    // Create venv
    QProcess process;
    QStringList args;
    args << "-m" << "venv" << venvPath;

    emit venvProgress(QString("Executing: %1 %2").arg(pythonExe, args.join(" ")));

    process.start(pythonExe, args);
    process.waitForFinished(60000); // 60 second timeout

    if (process.exitCode() != 0 || !venvExists())
    {
        QString error = process.readAllStandardError();
        emit outputReceived(QString("Failed to create virtual environment: %1").arg(error), true);
        return false;
    }

    emit venvProgress("Virtual environment created successfully");
    emit outputReceived("Virtual environment created at: " + venvPath, false);

    // Upgrade pip
    emit venvProgress("Upgrading pip...");
    if (!upgradePip())
    {
        emit outputReceived("Warning: Failed to upgrade pip", true);
    }

    // Install pip-tools
    emit venvProgress("Installing pip-tools...");
    if (!installPipTools())
    {
        emit outputReceived("Warning: Failed to install pip-tools", true);
    }

    emit venvProgress("Virtual environment setup complete");
    return true;
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
 * @brief Upgrades pip in the virtual environment.
 ***************************************************************/
bool TerminalEngine::upgradePip(const QString &version)
{
    if (!venvExists())
    {
        emit outputReceived("Virtual environment does not exist", true);
        return false;
    }

    QString pythonExe = getPythonExecutable();
    QStringList args;
    args << "-m" << "pip" << "install" << "--upgrade";

    if (version.isEmpty())
    {
        args << "pip";
    }
    else
    {
        args << QString("pip==%1").arg(version);
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
    if (command.trimmed().isEmpty())
    {
        emit outputReceived("No command entered", true);
        return;
    }

    currentCommand = command.trimmed();
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
    QString cmd = command.trimmed();

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

/************** End of TerminalEngine.cpp ***********************/
