/******************************************************************
 * File: MainWindow.cpp
 * Author: Jeffrey Scott Flesher
 * Description:
 *   Implements the main application window for PipMatrixResolverQt.
 *   Provides requirements.txt loading (local & web),
 *   and integration with ResolverEngine, VenvManager, BatchRunner,
 *   MatrixHistory, and MatrixUtility.
 *
 * Version: 0.5
 * Date:    2025-11-01
 ******************************************************************/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ResolverEngine.h"
#include "VenvManager.h"
#include "BatchRunner.h"
#include "MatrixHistory.h"
#include "MatrixUtility.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QTextBrowser>
#include <QDialog>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QStandardItemModel>

/******************************************************************
 * Constructor
 ******************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    resolver(new ResolverEngine(this)),
    venv(new VenvManager(this)),
    batch(new BatchRunner(this)),
    requirementsModel(new QStandardItemModel(this)),
    hasValidRequirements(false)
{
    ui->setupUi(this);

    // Assign pointers from ui
    requirementsView = ui->requirementsView;
    matrixView       = ui->matrixView;
    logView          = ui->logView;
    progress         = ui->progressBar;
    historyWidget    = ui->matrixHistoryWidget;

    bindSignals();
    applyToolsEnabled(false);
    statusBar()->showMessage(tr("Ready"));

    // Connect menu/toolbar actions defined in .ui
    connect(ui->actionAbout, &QAction::triggered,
            this, &MainWindow::showAboutBox);
    connect(ui->actionViewReadme, &QAction::triggered,
            this, &MainWindow::showReadmeDialog);
    connect(ui->actionExit, &QAction::triggered,
            this, &MainWindow::exitApp);

    connect(ui->actionOpenRequirements, &QAction::triggered,
            this, &MainWindow::openLocalRequirements);
    connect(ui->actionFetchRequirements, &QAction::triggered,
            this, &MainWindow::fetchRequirementsFromUrl);

    connect(ui->actionCreateVenv, &QAction::triggered,
            this, &MainWindow::createOrUpdateVenv);
    connect(ui->actionResolveMatrix, &QAction::triggered,
            this, &MainWindow::startResolve);
    connect(ui->actionPause, &QAction::triggered,
            this, &MainWindow::pauseResolve);
    connect(ui->actionResume, &QAction::triggered,
            this, &MainWindow::resumeResolve);
    connect(ui->actionStop, &QAction::triggered,
            this, &MainWindow::stopResolve);

    connect(ui->actionRunBatch, &QAction::triggered,
            this, &MainWindow::runBatch);
}

/******************************************************************
 * Destructor
 ******************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
}

/******************************************************************
 * bindSignals
 ******************************************************************/
void MainWindow::bindSignals()
{
    connect(resolver, &ResolverEngine::logMessage,
            this, &MainWindow::appendLog);
    connect(resolver, &ResolverEngine::progressChanged,
            this, &MainWindow::updateProgress);
    connect(resolver, &ResolverEngine::successCompiled,
            this, &MainWindow::showCompiledResult);

    connect(venv, &VenvManager::logMessage,
            this, &MainWindow::appendLog);

    connect(batch, &BatchRunner::logMessage,
            this, &MainWindow::appendLog);
    connect(batch, &BatchRunner::progressChanged,
            this, &MainWindow::updateProgress);
    connect(batch, &BatchRunner::allJobsFinished,
            this, [this]() {
                statusBar()->showMessage(tr("Batch jobs complete"));
            });
}

/******************************************************************
 * openMatrixHistory
 ******************************************************************/
void MainWindow::openMatrixHistory()
{
    // Local history import
    connect(historyWidget, &MatrixHistory::localHistoryImported,
            this, [this](const QString &path) {
                QStringList lines = MatrixUtility::readTextFileLines(path);
                MatrixUtility::writeTableToModel(requirementsModel, lines);
                MatrixUtility::ensureViewScrollable(requirementsView);
                applyToolsEnabled(true);
                appendLog(tr("Imported requirements from local history: %1").arg(path));
                ui->mainTabs->setCurrentWidget(ui->tabMain);
            });

    // Web history import
    connect(historyWidget, &MatrixHistory::webHistoryImported,
            this, [this](const QString &url) {
                QByteArray content;
                if (!MatrixUtility::downloadText(url, content)) {
                    QMessageBox::warning(this, tr("Download failed"),
                                         tr("Failed to fetch requirements from URL:\n%1").arg(url));
                    return;
                }
                const QStringList lines = QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);
                MatrixUtility::writeTableToModel(requirementsModel, lines);
                MatrixUtility::ensureViewScrollable(requirementsView);
                applyToolsEnabled(true);
                appendLog(tr("Imported requirements from web history: %1").arg(url));
                ui->mainTabs->setCurrentWidget(ui->tabMain);
            });

    // Exit back to main window
    connect(historyWidget, &MatrixHistory::exitRequested,
            this, [this]() {
                ui->mainTabs->setCurrentWidget(ui->tabMain);
            });

    // Clear history → clear menus too
    connect(historyWidget, &MatrixHistory::historyCleared,
            this, [this]() {
                historyRecentLocal.clear();
                historyRecentWeb.clear();
                refreshRecentMenus();
            });

    ui->mainTabs->setCurrentWidget(ui->tabHistory);
}
/******************************************************************
 * openLocalRequirements
 ******************************************************************/
void MainWindow::openLocalRequirements()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open requirements.txt"),
        QString(),
        tr("Text Files (*.txt)")
        );

    if (path.isEmpty())
        return;

    const QStringList lines = MatrixUtility::readTextFileLines(path);

    if (lines.isEmpty())
    {
        QMessageBox::warning(
            this,
            tr("Empty file"),
            tr("The selected file is empty or unreadable.")
            );
        return;
    }

    MatrixUtility::writeTableToModel(requirementsModel, lines);
    MatrixUtility::ensureViewScrollable(requirementsView);
    applyToolsEnabled(true);
    appendLog(tr("Loaded requirements (local): %1").arg(path));
}

/******************************************************************
 * fetchRequirementsFromUrl
 ******************************************************************/
void MainWindow::fetchRequirementsFromUrl()
{
    bool ok = false;
    QString inputUrl = QInputDialog::getText(
        this,
        tr("Fetch requirements"),
        tr("Enter URL:"),
        QLineEdit::Normal,
        "",
        &ok
        );

    if (!ok || inputUrl.isEmpty())
        return;

    const QString rawUrl = MatrixUtility::normalizeRawUrl(inputUrl);

    QByteArray content;
    if (!MatrixUtility::downloadText(rawUrl, content))
    {
        QMessageBox::warning(
            this,
            tr("Download failed"),
            tr("Failed to fetch requirements from URL:\n%1").arg(rawUrl)
            );
        return;
    }

    const QStringList lines = QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);
    MatrixUtility::writeTableToModel(requirementsModel, lines);
    MatrixUtility::ensureViewScrollable(requirementsView);
    applyToolsEnabled(true);
    appendLog(tr("Fetched requirements from URL: %1").arg(rawUrl));
}

/******************************************************************
 * createOrUpdateVenv
 ******************************************************************/
void MainWindow::createOrUpdateVenv()
{
    appendLog(tr("Creating or updating virtual environment..."));
    venv->createOrUpdate();
}

/******************************************************************
 * startResolve
 ******************************************************************/
void MainWindow::startResolve()
{
    appendLog(tr("Starting matrix resolution..."));
    resolver->start();
}

/******************************************************************
 * pauseResolve
 ******************************************************************/
void MainWindow::pauseResolve()
{
    appendLog(tr("Pausing..."));
    resolver->pause();
}

/******************************************************************
 * resumeResolve
 ******************************************************************/
void MainWindow::resumeResolve()
{
    appendLog(tr("Resuming..."));
    resolver->resume();
}

/******************************************************************
 * stopResolve
 ******************************************************************/
void MainWindow::stopResolve()
{
    appendLog(tr("Stopping..."));
    resolver->stop();
}

/******************************************************************
 * runBatch
 ******************************************************************/
void MainWindow::runBatch()
{
    BatchJob job;
    job.imagePath = QFileDialog::getOpenFileName(
        this,
        tr("Select image"),
        QString(),
        tr("Images (*.png *.jpg *.jpeg)")
        );

    if (job.imagePath.isEmpty())
        return;

    job.audioPath = QFileDialog::getOpenFileName(
        this,
        tr("Select audio"),
        QString(),
        tr("Audio Files (*.wav *.mp3)")
        );

    if (job.audioPath.isEmpty())
        return;

    QString baseName = QFileInfo(job.imagePath).completeBaseName();
    QString outFile = QDir(MatrixUtility::logsDir()).filePath(baseName + "_output.mp4");

    job.outputPath = outFile;

    batch->enqueue(job);
    batch->start();
}

/******************************************************************
 * appendLog
 ******************************************************************/
void MainWindow::appendLog(const QString &line)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    logView->appendPlainText(QString("[%1] %2").arg(time, line));
}

/******************************************************************
 * updateProgress
 ******************************************************************/
void MainWindow::updateProgress(int percent)
{
    progress->setValue(percent);
}

/******************************************************************
 * showCompiledResult
 ******************************************************************/
void MainWindow::showCompiledResult(const QString &path)
{
    appendLog(tr("Successfully compiled: %1").arg(path));
    statusBar()->showMessage(tr("Compiled successfully"));
}

/******************************************************************
 * showAboutBox
 ******************************************************************/
void MainWindow::showAboutBox()
{
    QMessageBox::about(
        this,
        tr("About Pip Matrix Resolver"),
        tr("<b>Pip Matrix Resolver</b><br>"
           "Cross-platform Qt tool to resolve "
           "Python dependency matrices.<br>"
           "Version 0.5")
        );
}

/******************************************************************
 * showReadmeDialog
 ******************************************************************/
void MainWindow::showReadmeDialog()
{
    QFile file(":/docs/README.md");
    QString markdown;

    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        markdown = QString::fromUtf8(file.readAll());
    else
        markdown = tr("README.md not found in resources.");

    QDialog dialog(this);
    dialog.setWindowTitle(tr("README"));
    dialog.resize(700, 500);

    QVBoxLayout layout(&dialog);
    QTextBrowser viewer(&dialog);
    viewer.setMarkdown(markdown);
    viewer.setOpenExternalLinks(true);

    QPushButton closeButton(tr("Close"), &dialog);
    QObject::connect(&closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    layout.addWidget(&viewer);
    layout.addWidget(&closeButton);
    dialog.setLayout(&layout);

    dialog.exec();
}

/******************************************************************
 * exitApp
 ******************************************************************/
void MainWindow::exitApp()
{
    close();
}

/******************************************************************
 * applyToolsEnabled
 ******************************************************************/
void MainWindow::applyToolsEnabled(bool enabled)
{
    hasValidRequirements = enabled;
    if (ui->actionCreateVenv)    ui->actionCreateVenv->setEnabled(enabled);
    if (ui->actionResolveMatrix) ui->actionResolveMatrix->setEnabled(enabled);
    if (ui->actionPause)         ui->actionPause->setEnabled(enabled);
    if (ui->actionResume)        ui->actionResume->setEnabled(enabled);
    if (ui->actionStop)          ui->actionStop->setEnabled(enabled);
}

/******************************************************************
 * refreshRecentMenus
 ******************************************************************/
void MainWindow::refreshRecentMenus()
{
    if (recentLocalMenu) {
        recentLocalMenu->clear();
        for (const QString &path : historyRecentLocal)
        {
            recentLocalMenu->addAction(path, this, [this, path]()
            {
                QStringList lines = MatrixUtility::readTextFileLines(path);
                MatrixUtility::writeTableToModel(requirementsModel, lines);
                MatrixUtility::ensureViewScrollable(requirementsView);
                applyToolsEnabled(true);
                appendLog(tr("Loaded requirements (recent local): %1").arg(path));
            });
        }
    }

    if (recentWebMenu)
    {
        recentWebMenu->clear();
        for (const QString &url : historyRecentWeb)
        {
            recentWebMenu->addAction(url, this, [this, url]()
            {
                QByteArray content;
                if (!MatrixUtility::downloadText(url, content))
                {
                    QMessageBox::warning(this, tr("Download failed"), tr("Failed to fetch requirements from URL:\n%1").arg(url));
                    return;
                }
                const QStringList lines = QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);
                MatrixUtility::writeTableToModel(requirementsModel, lines);
                MatrixUtility::ensureViewScrollable(requirementsView);
                applyToolsEnabled(true);
                appendLog(tr("Loaded requirements (recent web): %1").arg(url));
            });
        }
    }
}

// In MainWindow.cpp
void MainWindow::addDummyHistory()
{
    historyRecentLocal << "C:/temp/requirements1.txt"
                       << "C:/temp/requirements2.txt";
    historyRecentWeb   << "https://example.com/reqs1.txt"
                     << "https://example.com/reqs2.txt";
    refreshRecentMenus();
}

/************** End of MainWindow.cpp *************************/
