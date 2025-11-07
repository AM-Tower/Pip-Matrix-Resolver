/****************************************************************
 * @file CommandsTab.cpp
 * @brief Implements the CommandsTab class for dynamic command UI,
 *        batch execution, project editing, and JSON persistence.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.7
 * @date    2025-11-03
 * @section License MIT
 * @section DESCRIPTION
 * This file contains the implementation of CommandsTab widget.
 ***************************************************************/
#include "CommandsTab.h"
#include "MainWindow.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QTextStream>

/****************************************************************
 * @brief Constructor: Builds the UI and loads initial state.
 ***************************************************************/
CommandsTab::CommandsTab(TerminalEngine* engine, QWidget* parent)
    : QWidget(parent), engine(engine)
{
    buildUI();
    loadProjects("projects.json");
}
/****************************************************************
 * @brief Builds the static UI components.
 ***************************************************************/
void CommandsTab::buildUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Project selector and management buttons
    QHBoxLayout *projLayout = new QHBoxLayout;
    projectDropdown = new QComboBox;
    connect(projectDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CommandsTab::onProjectChanged);

    addProjectButton = new QPushButton("Add");
    editProjectButton = new QPushButton("Edit");
    deleteProjectButton = new QPushButton("Delete");

    connect(addProjectButton, &QPushButton::clicked, this, &CommandsTab::onAddProject);
    connect(editProjectButton, &QPushButton::clicked, this, &CommandsTab::onEditProject);
    connect(deleteProjectButton, &QPushButton::clicked, this, &CommandsTab::onDeleteProject);

    projLayout->addWidget(new QLabel("Project:"));
    projLayout->addWidget(projectDropdown);
    projLayout->addWidget(addProjectButton);
    projLayout->addWidget(editProjectButton);
    projLayout->addWidget(deleteProjectButton);
    mainLayout->addLayout(projLayout);

    // Dynamic inputs area
    inputsLayout = new QVBoxLayout;
    mainLayout->addLayout(inputsLayout);

    // Extra arguments
    extraArgsEdit = new QLineEdit;
    connect(extraArgsEdit, &QLineEdit::textChanged, this, &CommandsTab::updatePreview);
    mainLayout->addWidget(new QLabel("Extra Arguments:"));
    mainLayout->addWidget(extraArgsEdit);

    // Command preview + clear button
    QHBoxLayout *previewLayout = new QHBoxLayout;
    commandPreview = new QLineEdit;
    commandPreview->setReadOnly(true);
    clearButton = new QToolButton;
    clearButton->setText("X");
    connect(clearButton, &QToolButton::clicked, this, &CommandsTab::onClearCommand);
    previewLayout->addWidget(commandPreview);
    previewLayout->addWidget(clearButton);
    mainLayout->addLayout(previewLayout);

    // Batch file selector
    QHBoxLayout *batchLayout = new QHBoxLayout;
    batchFileEdit = new QLineEdit;
    batchFileEdit->setPlaceholderText("Batch file (one path per line)");
    QPushButton *browseBatchBtn = new QPushButton("Browse");
    connect(browseBatchBtn, &QPushButton::clicked, this, &CommandsTab::onBrowseBatchFile);
    batchLayout->addWidget(batchFileEdit);
    batchLayout->addWidget(browseBatchBtn);
    mainLayout->addLayout(batchLayout);

    // Output console
    outputConsole = new QTextEdit;
    outputConsole->setReadOnly(true);
    mainLayout->addWidget(new QLabel("Command Output:"));
    mainLayout->addWidget(outputConsole);

    // Run buttons
    QHBoxLayout *runLayout = new QHBoxLayout;
    runButton = new QPushButton("Run Command");
    runBatchButton = new QPushButton("Run Batch");
    connect(runButton, &QPushButton::clicked, this, &CommandsTab::onRunCommand);
    connect(runBatchButton, &QPushButton::clicked, this, &CommandsTab::onRunBatch);
    runLayout->addWidget(runButton);
    runLayout->addWidget(runBatchButton);
    mainLayout->addLayout(runLayout);
}

/****************************************************************
 * @brief Loads projects from a JSON file.
 ***************************************************************/
bool CommandsTab::loadProjects(const QString &jsonPath)
{
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray projArray = doc["projects"].toArray();

    projects.clear();
    projectDropdown->clear();

    for (int i = 0; i < projArray.size(); ++i)
    {
        QJsonObject obj = projArray.at(i).toObject();
        ProjectDef proj;
        proj.name = obj["name"].toString();
        proj.scriptPath = obj["script"].toString();
        proj.extraArgs = obj["extra_args"].toString();

        QJsonArray inputsArr = obj["inputs"].toArray();
        for (int j = 0; j < inputsArr.size(); ++j)
        {
            QJsonObject inObj = inputsArr.at(j).toObject();
            proj.inputs.append({inObj["label"].toString(), inObj["switch"].toString()});
        }

        projects.append(proj);
        projectDropdown->addItem(proj.name);
    }
    if (!projects.isEmpty())
    {
        onProjectChanged(0);
    }
    return true;
}

/****************************************************************
 * @brief Saves projects to a JSON file.
 ***************************************************************/
bool CommandsTab::saveProjects(const QString &jsonPath)
{
    QJsonArray arr;
    for (int i = 0; i < projects.size(); ++i)
    {
        const ProjectDef &proj = projects.at(i);
        QJsonObject obj;
        obj["name"] = proj.name;
        obj["script"] = proj.scriptPath;
        obj["extra_args"] = proj.extraArgs;
        QJsonArray inputsArr;
        for (int j = 0; j < proj.inputs.size(); ++j)
        {
            const InputDef &input = proj.inputs.at(j);
            QJsonObject inObj;
            inObj["label"] = input.label;
            inObj["switch"] = input.switchName;
            inputsArr.append(inObj);
        }
        obj["inputs"] = inputsArr;
        arr.append(obj);
    }
    QJsonObject root;
    root["projects"] = arr;
    QFile file(jsonPath);
    if (!file.open(QIODevice::WriteOnly))
    {
        return false;
    }
    file.write(QJsonDocument(root).toJson());
    return true;
}

/****************************************************************
 * @brief Handles project selection change.
 ***************************************************************/
void CommandsTab::onProjectChanged(int index)
{
    if (index < 0 || index >= projects.size())
    {
        return;
    }
    rebuildInputs(projects.at(index));
    extraArgsEdit->setText(projects.at(index).extraArgs);
    updatePreview();
}

/****************************************************************
 * @brief Rebuilds input fields dynamically.
 ***************************************************************/
void CommandsTab::rebuildInputs(const ProjectDef &proj)
{
    QLayoutItem *child;
    while ((child = inputsLayout->takeAt(0)) != nullptr)
    {
        delete child->widget();
        delete child;
    }
    inputEdits.clear();

    for (int i = 0; i < proj.inputs.size(); ++i)
    {
        const InputDef &input = proj.inputs.at(i);
        auto lbl = new QLabel(input.label);
        auto edit = new QLineEdit;
        connect(edit, &QLineEdit::textChanged, this, &CommandsTab::updatePreview);
        inputsLayout->addWidget(lbl);
        inputsLayout->addWidget(edit);
        inputEdits.append(edit);
    }
}

QString CommandsTab::buildCommand() const
{
    int index = projectDropdown->currentIndex();
    if (index < 0 || index >= projects.size())
        return QString();

    const ProjectDef& proj = projects.at(index);

    // Use engine’s resolved interpreter
    QString pythonExe = engine->pythonCommand();
    QStringList args;

    args << QString("\"%1\"").arg(proj.scriptPath);

    for (int i = 0; i < inputEdits.size(); ++i)
    {
        QString val = inputEdits.at(i)->text().trimmed();
        if (!val.isEmpty())
        {
            args << proj.inputs.at(i).switchName << QString("\"%1\"").arg(val);
        }
    }

    if (!extraArgsEdit->text().isEmpty())
    {
        args << extraArgsEdit->text();
    }

    return QString("%1 %2").arg(pythonExe, args.join(' '));
}

/****************************************************************
 * @brief Validates script and input files exist.
 ***************************************************************/
bool CommandsTab::validateFiles(QString &errorMsg) const
{
    int index = projectDropdown->currentIndex();
    if (index < 0 || index >= projects.size())
    {
        errorMsg = "Invalid project selected.";
        return false;
    }

    QFileInfo scriptInfo(projects.at(index).scriptPath);
    if (!scriptInfo.exists())
    {
        errorMsg = "Script file does not exist.";
        return false;
    }

    for (int i = 0; i < inputEdits.size(); ++i)
    {
        if (!inputEdits.at(i)->text().isEmpty())
        {
            QFileInfo fi(inputEdits.at(i)->text());
            if (!fi.exists())
            {
                errorMsg = QString("Input file missing: %1").arg(inputEdits.at(i)->text());
                return false;
            }
        }
    }
    return true;
}

/****************************************************************
 * @brief Updates the command preview.
 ***************************************************************/
void CommandsTab::updatePreview()
{
    commandPreview->setText(buildCommand());
}

/****************************************************************
 * @brief Runs the command using QProcess.
 ***************************************************************/
void CommandsTab::onRunCommand()
{
    QString errorMsg;
    if (!validateFiles(errorMsg))
    {
        QMessageBox::critical(this, "Validation Error", errorMsg);
        return;
    }

    QString cmd = buildCommand();
    outputConsole->append(QString("Running: %1").arg(cmd));
    executeCommand(cmd);
}

/****************************************************************
 * @brief Runs batch commands sequentially from a file.
 ***************************************************************/
void CommandsTab::onRunBatch()
{
    QString batchFile = batchFileEdit->text();
    QFile file(batchFile);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Error", "Cannot open batch file.");
        return;
    }

    QTextStream in(&file);
    batchQueue.clear();

    int index = projectDropdown->currentIndex();
    if (index < 0 || index >= projects.size())
    {
        QMessageBox::critical(this, "Error", "No project selected.");
        return;
    }
    const ProjectDef &proj = projects.at(index);

    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        // Fill inputs from line
        QStringList batchInputs = line.split(' ', Qt::SkipEmptyParts);
        for (int i = 0; i < inputEdits.size(); ++i)
        {
            if (i < batchInputs.size())
                inputEdits.at(i)->setText(batchInputs.at(i));
            else
                inputEdits.at(i)->clear();
        }

        QString cmd = buildCommand();
        batchQueue.append(cmd);
    }

    outputConsole->append(QString("Queued %1 batch commands").arg(batchQueue.size()));
    runNextBatchCommand();
}

/****************************************************************
 * @brief Runs the next command in the batch queue.
 ***************************************************************/
void CommandsTab::runNextBatchCommand()
{
    if (batchQueue.isEmpty())
    {
        outputConsole->append("Batch execution finished.");
        return;
    }

    QString cmd = batchQueue.takeFirst();
    outputConsole->append(QString("Batch Running: %1").arg(cmd));

    // Use venv Python path from engine
    QString venvPython = engine->venvPythonPath(engine->venvPath);
    QStringList parts = QProcess::splitCommand(cmd);

    // Replace "python" with venv python
    if (!parts.isEmpty() && parts.first().toLower() == "python")
    {
        parts[0] = venvPython;
    }

    QString program = parts.takeFirst();

    batchProc = new QProcess(this);
    batchProc->setProcessChannelMode(QProcess::MergedChannels);

    connect(batchProc, &QProcess::readyReadStandardOutput, [this]()
            {
                outputConsole->append(batchProc->readAllStandardOutput());
            });
    connect(batchProc, &QProcess::readyReadStandardError, [this]()
            {
                outputConsole->append(batchProc->readAllStandardError());
            });
    connect(batchProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this](int exitCode, QProcess::ExitStatus status)
            {
                outputConsole->append(QString("Process finished with code %1, status %2")
                                          .arg(exitCode).arg(status));
                batchProc->deleteLater();
                batchProc = nullptr;
                runNextBatchCommand(); // run next queued command
            });

    batchProc->start(program, parts);
}

/****************************************************************
 * @brief Executes a command using QProcess with program + args.
 ***************************************************************/
/****************************************************************
 * @brief Executes a command using QProcess with program + args.
 ***************************************************************/
void CommandsTab::executeCommand(const QString &cmd)
{
    // Split program and args safely
    QStringList parts = QProcess::splitCommand(cmd);
    if (parts.isEmpty())
    {
        showStatusMessage("Invalid command string", 5000);
        return;
    }

    QString program = parts.takeFirst();
    QProcess *proc = new QProcess(this);

    // Merge stdout + stderr into one channel
    proc->setProcessChannelMode(QProcess::MergedChannels);

    // Only connect to standard output in merged mode
    connect(proc, &QProcess::readyReadStandardOutput, [this, proc]()
            {
                outputConsole->append(proc->readAllStandardOutput());
            });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            [this, proc](int exitCode, QProcess::ExitStatus status)
            {
                outputConsole->append(
                    QString("Process finished with code %1, status %2")
                        .arg(exitCode)
                        .arg(status)
                    );
                proc->deleteLater();
            });

    proc->start(program, parts);
}

void CommandsTab::showStatusMessage(const QString& msg, int timeoutMs)
{
    // Find the MainWindow that owns this tab
    MainWindow* mw = qobject_cast<MainWindow*>(window());
    if (mw && mw->statusBar)
    {
        mw->statusBar->showMessage(msg, timeoutMs);
    }
}

/****************************************************************
 * @brief Clears the command preview.
 ***************************************************************/
void CommandsTab::onClearCommand()
{
    commandPreview->clear();
}

/****************************************************************
 * @brief Opens a file dialog to select batch file.
 ***************************************************************/
void CommandsTab::onBrowseBatchFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Select Batch File", QString(), "Text Files (*.txt);;All Files (*)");
    if (!fileName.isEmpty())
    {
        batchFileEdit->setText(fileName);
    }
}

/****************************************************************
 * @brief Shows the project editor dialog for add/edit.
 ***************************************************************/
bool CommandsTab::showProjectDialog(ProjectDef &proj, bool isEdit)
{
    QDialog dialog(this);
    dialog.setWindowTitle(isEdit ? "Edit Project" : "Add Project");
    QFormLayout *form = new QFormLayout(&dialog);

    QLineEdit *nameEdit = new QLineEdit(proj.name, &dialog);
    QLineEdit *scriptEdit = new QLineEdit(proj.scriptPath, &dialog);
    QLineEdit *extraArgsEditDlg = new QLineEdit(proj.extraArgs, &dialog);
    QSpinBox *inputCountSpin = new QSpinBox(&dialog);
    inputCountSpin->setRange(1, 10);
    inputCountSpin->setValue(proj.inputs.isEmpty() ? 2 : proj.inputs.size());

    QVector<QLineEdit*> labelEdits;
    QVector<QLineEdit*> switchEdits;
    QVBoxLayout *inputsVBox = new QVBoxLayout;
    for (int i = 0; i < inputCountSpin->value(); ++i)
    {
        QHBoxLayout *row = new QHBoxLayout;
        QLineEdit *lblEdit = new QLineEdit(i < proj.inputs.size() ? proj.inputs.at(i).label : QString("input%1").arg(i+1), &dialog);
        QLineEdit *swEdit = new QLineEdit(i < proj.inputs.size() ? proj.inputs.at(i).switchName : "", &dialog);
        row->addWidget(new QLabel("Label:"));
        row->addWidget(lblEdit);
        row->addWidget(new QLabel("Switch:"));
        row->addWidget(swEdit);
        inputsVBox->addLayout(row);
        labelEdits.append(lblEdit);
        switchEdits.append(swEdit);
    }
    connect(inputCountSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [inputsVBox, &labelEdits, &switchEdits, &dialog](int count)
            {
                QLayoutItem *child;
                while ((child = inputsVBox->takeAt(0)) != nullptr)
                {
                    delete child->widget();
                    delete child;
                }
                labelEdits.clear();
                switchEdits.clear();
                for (int i = 0; i < count; ++i)
                {
                    QHBoxLayout *row = new QHBoxLayout;
                    QLineEdit *lblEdit = new QLineEdit(QString("input%1").arg(i+1), &dialog);
                    QLineEdit *swEdit = new QLineEdit("", &dialog);
                    row->addWidget(new QLabel("Label:"));
                    row->addWidget(lblEdit);
                    row->addWidget(new QLabel("Switch:"));
                    row->addWidget(swEdit);
                    inputsVBox->addLayout(row);
                    labelEdits.append(lblEdit);
                    switchEdits.append(swEdit);
                }
            }
            );

    form->addRow("Project Name:", nameEdit);
    form->addRow("Script Path:", scriptEdit);
    form->addRow("Extra Args:", extraArgsEditDlg);
    form->addRow("Number of Inputs:", inputCountSpin);
    form->addRow("Inputs:", inputsVBox);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted)
    {
        proj.name = nameEdit->text();
        proj.scriptPath = scriptEdit->text();
        proj.extraArgs = extraArgsEditDlg->text();
        proj.inputs.clear();
        for (int i = 0; i < labelEdits.size(); ++i)
        {
            proj.inputs.append({labelEdits.at(i)->text(), switchEdits.at(i)->text()});
        }
        return true;
    }
    return false;
}

/****************************************************************
 * @brief Adds a new project.
 ***************************************************************/
void CommandsTab::onAddProject()
{
    ProjectDef proj;
    if (showProjectDialog(proj, false))
    {
        projects.append(proj);
        saveProjects("projects.json");
        refreshProjectDropdown();
        projectDropdown->setCurrentIndex(projects.size() - 1);
    }
}

/****************************************************************
 * @brief Edits the selected project.
 ***************************************************************/
void CommandsTab::onEditProject()
{
    int index = projectDropdown->currentIndex();
    if (index < 0 || index >= projects.size())
    {
        return;
    }

    ProjectDef proj = projects.at(index);
    if (showProjectDialog(proj, true))
    {
        projects[index] = proj;
        saveProjects("projects.json");
        refreshProjectDropdown();
        projectDropdown->setCurrentIndex(index);
    }
}

/****************************************************************
 * @brief Deletes the selected project.
 ***************************************************************/
void CommandsTab::onDeleteProject()
{
    int index = projectDropdown->currentIndex();
    if (index < 0 || index >= projects.size())
    {
        return;
    }
    if (QMessageBox::question(this, "Delete Project", "Are you sure you want to delete this project?") == QMessageBox::Yes)
    {
        projects.remove(index);
        saveProjects("projects.json");
        refreshProjectDropdown();
        if (!projects.isEmpty())
        {
            projectDropdown->setCurrentIndex(0);
        }
    }
}

/****************************************************************
 *es.
 ***************************************************************/
void CommandsTab::refreshProjectDropdown()
{
    projectDropdown->clear();
    for (int i = 0; i < projects.size(); ++i)
    {
        projectDropdown->addItem(projects.at(i).name);
    }
}
/************** End of CommandsTab.cpp **************************/
