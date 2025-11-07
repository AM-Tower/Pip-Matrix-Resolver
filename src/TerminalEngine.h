/****************************************************************
 * @file TerminalEngine.h
 * @brief Declares the TerminalEngine class for terminal operations.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.8
 * @date    2025-11-04
 * @section License MIT
 * @section DESCRIPTION
 * This file defines the TerminalEngine class for managing virtual
 * environments, executing commands, and handling terminal I/O.
 * Features:
 *   - Virtual environment creation and management
 *   - Cross-platform command execution
 *   - Real-time output streaming
 *   - Python, pip, and pip-tools command support
 *   - Shell command execution
 ***************************************************************/
#ifndef TERMINALENGINE_H
#define TERMINALENGINE_H

#include <QObject>
#include <QString>
#include <QProcess>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDateTime>

/****************************************************************
 * @class TerminalEngine
 * @brief Manages terminal operations and virtual environments.
 ***************************************************************/
class TerminalEngine : public QObject
{
    Q_OBJECT

public:
    explicit TerminalEngine(QObject *parent = nullptr);
    ~TerminalEngine();

    /****************************************************************
     * @brief Sets Python command globally.
     * @param Gets version From Settings.
     ***************************************************************/
    static void setPythonCommand(const QString &versionFromSettings);
    static QStringList pythonBaseArgs();
    /****************************************************************
     * @brief Sets the virtual environment path.
     * @param venvPath Absolute path to the virtual environment.
     ***************************************************************/
    void setVenvPath(const QString &venvPath);

    /****************************************************************
     * @brief Gets the current virtual environment path.
     * @return The virtual environment path.
     ***************************************************************/
    QString getVenvPath() const;

    /****************************************************************
     * @brief Creates a new virtual environment.
     * @param pythonVersion Python version to use (e.g., "3.11").
     * @return true if successful, false otherwise.
     ***************************************************************/
    bool createVirtualEnvironment(const QString &pythonVersion);

    /****************************************************************
     * @brief Checks if virtual environment exists.
     * @return true if venv exists, false otherwise.
     ***************************************************************/
    bool venvExists() const;

    /****************************************************************
     * @brief Activates an existing virtual environment.
     * @return true if successful, false otherwise.
     ***************************************************************/
    bool activateVenv();

    /****************************************************************
     * @brief Gets venv status information.
     * @return Status string describing venv state.
     ***************************************************************/
    QString getVenvStatus() const;

    /****************************************************************
     * @brief Upgrades pip in the virtual environment.
     * @return true if successful, false otherwise.
     ***************************************************************/
    bool upgradePip();

    /****************************************************************
     * @brief Installs pip-tools in the virtual environment.
     * @param version Specific pip-tools version or empty for latest.
     * @return true if successful, false otherwise.
     ***************************************************************/
    bool installPipTools(const QString &version = QString());

    /****************************************************************
     * @brief Executes a command in the terminal.
     * @param command The command to execute.
     ***************************************************************/
    void executeCommand(const QString &command);

    /****************************************************************
     * @brief Stops the currently running process.
     ***************************************************************/
    void stopCurrentProcess();

    /****************************************************************
     * @brief Gets the Python executable path for the venv.
     * @return Path to Python executable.
     ***************************************************************/
    QString getPythonExecutable() const;

    /****************************************************************
     * @brief Gets the pip executable path for the venv.
     * @return Path to pip executable.
     ***************************************************************/
    QString getPipExecutable() const;
    /****************************************************************
     * @brief Resolve the Python interpreter command for execution.
     * @return Validated Python interpreter command (name or path).
     ***************************************************************/
    QString pythonCommand() const;
    QString venvPythonPath(const QString &venvPath) const;
    QString venvPath;

signals:
    /****************************************************************
     * @brief Emitted when output is available.
     * @param output The output text.
     * @param isError true if error output, false for standard output.
     ***************************************************************/
    void outputReceived(const QString &output, bool isError);

    /****************************************************************
     * @brief Emitted when a command starts executing.
     * @param command The command being executed.
     ***************************************************************/
    void commandStarted(const QString &command);

    /****************************************************************
     * @brief Emitted when a command finishes.
     * @param exitCode The process exit code.
     * @param exitStatus The process exit status.
     ***************************************************************/
    void commandFinished(int exitCode, QProcess::ExitStatus exitStatus);

    /****************************************************************
     * @brief Emitted when venv creation progress updates.
     * @param message Progress message.
     ***************************************************************/
    void venvProgress(const QString &message);

private slots:
    void onReadyReadStandardOutput();
    void onReadyReadStandardError();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);

private:
    /****************************************************************
     * @brief Checks if a command is runnable by invoking --version.
     * @param command Interpreter command (name or absolute path).
     * @return true if process starts and returns exit code 0.
     ***************************************************************/
    bool isCommandRunnable(const QString& command) const;
    /****************************************************************
     * @brief Parses and routes the command to appropriate handler.
     * @param command The command to parse.
     ***************************************************************/
    void parseAndExecuteCommand(const QString &command);

    /****************************************************************
     * @brief Executes a pip command.
     * @param args Arguments for pip command.
     ***************************************************************/
    void executePipCommand(const QStringList &args);

    /****************************************************************
     * @brief Executes a pip-tools command (pip-compile, pip-sync).
     * @param tool The tool name (compile or sync).
     * @param args Additional arguments.
     ***************************************************************/
    void executePipToolsCommand(const QString &tool, const QStringList &args);

    /****************************************************************
     * @brief Executes a Python script.
     * @param args Arguments including script path.
     ***************************************************************/
    void executePythonScript(const QStringList &args);

    /****************************************************************
     * @brief Executes a shell command.
     * @param command The shell command.
     ***************************************************************/
    void executeShellCommand(const QString &command);

    /****************************************************************
     * @brief Removes existing virtual environment.
     * @return true if successful, false otherwise.
     ***************************************************************/
    bool removeVirtualEnvironment();

    /****************************************************************
     * @brief Gets the platform-specific shell.
     * @return Shell executable name.
     ***************************************************************/
    QString getShell() const;

    /****************************************************************
     * @brief Gets shell arguments for command execution.
     * @param command The command to execute.
     * @return List of shell arguments.
     ***************************************************************/
    QStringList getShellArgs(const QString &command) const;

    /****************************************************************
     * @brief Logs a message with timestamp.
     * @param message The message to log.
     * @param isError true if error message.
     ***************************************************************/
    void logMessage(const QString &message, bool isError = false);

    QProcess *currentProcess;
    QString currentCommand;
    static QString g_pythonExe;
    static QStringList g_pythonBaseArgs;

};

#endif // TERMINALENGINE_H
/************** End of TerminalEngine.h *************************/
