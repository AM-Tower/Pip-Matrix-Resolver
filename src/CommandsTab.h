/****************************************************************
 * @file CommandsTab.h
 * @brief Declares the CommandsTab class for dynamic command UI,
 *        batch execution, project editing, and JSON persistence.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.7
 * @date    2025-11-03
 * @section License MIT
 * @section DESCRIPTION
 * This file defines the CommandsTab widget for PipMatrixResolverQt.
 * Features:
 *   - Dynamic UI from JSON schema
 *   - Batch command execution
 *   - Project add/edit/delete dialogs
 *   - JSON save/load
 *   - Real-time output streaming
 *   - Doxygen headers and C-style braces
 ***************************************************************/
#ifndef COMMANDSTAB_H
#define COMMANDSTAB_H

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QProcess>
#include <QLabel>
#include <QVector>
#include <QDialog>
#include <QSpinBox>
#include <QFileDialog>
#include "TerminalEngine.h"

/****************************************************************
 * @struct InputDef
 * @brief Defines a single input argument for a command.
 ***************************************************************/
struct InputDef
{
    QString label;
    QString switchName;
};

/****************************************************************
 * @struct ProjectDef
 * @brief Defines a project with script, inputs, and extra args.
 ***************************************************************/
struct ProjectDef
{
    QString name;
    QString scriptPath;
    QVector<InputDef> inputs;
    QString extraArgs;
};

/****************************************************************
 * @class CommandsTab
 * @brief Implements the dynamic command management tab.
 ***************************************************************/
class CommandsTab : public QWidget
{
    Q_OBJECT

public:
    /************************************************************
     * @brief Constructor
     * @param engine Pointer to the TerminalEngine backend
     * @param parent Optional parent widget
     ************************************************************/
    explicit CommandsTab(TerminalEngine* engine, QWidget* parent = nullptr);
    bool loadProjects(const QString &jsonPath);
    bool saveProjects(const QString &jsonPath);
    /************************************************************
     * @brief Show a message in the parent MainWindow's status bar
     * @param msg      The message text
     * @param timeoutMs Duration in milliseconds (default 5000)
     ************************************************************/
    void showStatusMessage(const QString& msg, int timeoutMs = 5000);

signals:
    void requestStatusMessage(const QString& msg, int timeoutMs);

private slots:
    void onProjectChanged(int index);
    void onRunCommand();
    void onRunBatch();
    void onClearCommand();
    void onAddProject();
    void onEditProject();
    void onDeleteProject();
    void updatePreview();
    void onBrowseBatchFile();

private:
    void buildUI();
    void rebuildInputs(const ProjectDef &proj);
    QString buildCommand() const;
    bool validateFiles(QString &errorMsg) const;
    void executeCommand(const QString &cmd);

    // Project editor dialog helpers
    bool showProjectDialog(ProjectDef &proj, bool isEdit = false);
    void refreshProjectDropdown();
    void runNextBatchCommand();   ///< helper to run next queued command
    // Variables:
    QVector<ProjectDef> projects;
    QComboBox *projectDropdown;
    QVBoxLayout *inputsLayout;
    QVector<QLineEdit *> inputEdits;
    QLineEdit *extraArgsEdit;
    QLineEdit *commandPreview;
    QTextEdit *outputConsole;
    QLineEdit *batchFileEdit;
    QPushButton *runButton;
    QPushButton *runBatchButton;
    QPushButton *addProjectButton;
    QPushButton *editProjectButton;
    QPushButton *deleteProjectButton;
    QToolButton *clearButton;
    TerminalEngine* engine;

    QStringList batchQueue;       ///< queued batch commands
    QProcess* batchProc = nullptr; ///< active batch process

};

#endif // COMMANDSTAB_H
/************** End of CommandsTab.h ***************************/
