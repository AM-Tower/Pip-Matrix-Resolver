#include "MainWindow.h"
#include "ResolverEngine.h"
#include "VenvManager.h"
#include "BatchRunner.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QTableView>
#include <QDir>
#include <QDateTime>

static QString logsDir()
{
    QDir dir("logs");
    if (!dir.exists()) dir.mkpath(".");
    return dir.absolutePath();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), resolver(new ResolverEngine(this)), venv(new VenvManager(this)), batch(new BatchRunner(this))
{
    setupUi();
    setupMenus();
    setupToolbar();
    bindSignals();
    statusBar()->showMessage(tr("Ready"));
}

void MainWindow::setupUi()
{
    auto splitter = new QSplitter(this);
    requirementsView = new QTableView(splitter);
    matrixView = new QTableView(splitter);

    auto bottomSplitter = new QSplitter(Qt::Horizontal, this);
    logView = new QPlainTextEdit(bottomSplitter);
    logView->setReadOnly(true);
    progress = new QProgressBar(bottomSplitter);
    progress->setRange(0, 100);

    auto central = new QWidget(this);
    auto layout = new QVBoxLayout(central);
    layout->addWidget(splitter);
    layout->addWidget(bottomSplitter);
    setCentralWidget(central);
}

void MainWindow::setupMenus()
{
    auto fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(QIcon(":/resources/icons/open.svg"), tr("Open requirements file..."), this, &MainWindow::openLocalRequirements);
    fileMenu->addAction(QIcon(":/resources/icons/url.svg"), tr("Fetch requirements from URL..."), this, &MainWindow::fetchRequirementsFromUrl);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), this, &MainWindow::exitApp);

    auto toolsMenu = menuBar()->addMenu(tr("&Tools"));
    toolsMenu->addAction(QIcon(":/resources/icons/venv.svg"), tr("Create/Update venv"), this, &MainWindow::createOrUpdateVenv);
    toolsMenu->addAction(QIcon(":/resources/icons/resolve.svg"), tr("Resolve matrix"), this, &MainWindow::startResolve);
    toolsMenu->addAction(QIcon(":/resources/icons/pause.svg"), tr("Pause"), this, &MainWindow::pauseResolve);
    toolsMenu->addAction(QIcon(":/resources/icons/resume.svg"), tr("Resume"), this, &MainWindow::resumeResolve);
    toolsMenu->addAction(QIcon(":/resources/icons/stop.svg"), tr("Stop"), this, &MainWindow::stopResolve);

    auto batchMenu = menuBar()->addMenu(tr("&Batch"));
    batchMenu->addAction(QIcon(":/resources/icons/batch.svg"), tr("Run batch conversion to mp4"), this, &MainWindow::runBatch);

    auto helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(QIcon(":/resources/icons/info.svg"), tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About Pip Matrix Resolver"),
                           tr("<b>Pip Matrix Resolver</b><br>Cross-platform Qt tool to resolve Python dependency matrices."));
    });
}

void MainWindow::setupToolbar()
{
    auto toolbar = addToolBar(tr("Main Toolbar"));
    toolbar->addAction(QIcon(":/resources/icons/open.svg"), tr("Open requirements file..."), this, &MainWindow::openLocalRequirements);
    toolbar->addAction(QIcon(":/resources/icons/url.svg"), tr("Fetch requirements from URL..."), this, &MainWindow::fetchRequirementsFromUrl);
    toolbar->addSeparator();
    toolbar->addAction(QIcon(":/resources/icons/venv.svg"), tr("Create/Update venv"), this, &MainWindow::createOrUpdateVenv);
    toolbar->addAction(QIcon(":/resources/icons/resolve.svg"), tr("Resolve matrix"), this, &MainWindow::startResolve);
    toolbar->addAction(QIcon(":/resources/icons/pause.svg"), tr("Pause"), this, &MainWindow::pauseResolve);
    toolbar->addAction(QIcon(":/resources/icons/resume.svg"), tr("Resume"), this, &MainWindow::resumeResolve);
    toolbar->addAction(QIcon(":/resources/icons/stop.svg"), tr("Stop"), this, &MainWindow::stopResolve);
    toolbar->addAction(QIcon(":/resources/icons/batch.svg"), tr("Run batch conversion to mp4"), this, &MainWindow::runBatch);
}

void MainWindow::bindSignals()
{
    // Connect ResolverEngine signals
    connect(resolver, &ResolverEngine::logMessage, this, &MainWindow::appendLog);
    connect(resolver, &ResolverEngine::progressChanged, this, &MainWindow::updateProgress);
    connect(resolver, &ResolverEngine::successCompiled, this, &MainWindow::showCompiledResult);

    // Connect VenvManager signals
    connect(venv, &VenvManager::logMessage, this, &MainWindow::appendLog);

    // Connect BatchRunner signals
    connect(batch, &BatchRunner::logMessage, this, &MainWindow::appendLog);
    connect(batch, &BatchRunner::progressChanged, this, &MainWindow::updateProgress);
    connect(batch, &BatchRunner::allJobsFinished, this, [this]() {
        statusBar()->showMessage(tr("Batch jobs complete"));
    });

}

void MainWindow::openLocalRequirements()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Open requirements.txt"), QString(), tr("Text Files (*.txt)"));
    if (path.isEmpty())
        return;
    appendLog(tr("Loaded requirements: %1").arg(path));
    resolver->loadRequirementsFromFile(path);
}

void MainWindow::fetchRequirementsFromUrl()
{
    bool ok = false;
    QString url = QInputDialog::getText(this, tr("Fetch requirements"), tr("Enter URL:"), QLineEdit::Normal, "", &ok);
    if (ok && !url.isEmpty())
    {
        appendLog(tr("Fetching requirements from: %1").arg(url));
        resolver->loadRequirementsFromUrl(url);
    }
}

void MainWindow::createOrUpdateVenv()
{
    QString dir = QDir("venvs").filePath("compile");
    appendLog(tr("Creating or updating virtual environment at %1").arg(dir));
    if (venv->createVenv(dir, "3"))
    {
        appendLog(tr("Virtual environment created."));
        venv->upgradePip("24.0", "7.4.0");
    }
    else
    {
        appendLog(tr("Failed to create virtual environment."));
    }
}

void MainWindow::startResolve()
{
    appendLog(tr("Starting matrix resolution..."));
    resolver->start();
}

void MainWindow::pauseResolve()
{
    appendLog(tr("Pausing..."));
    resolver->pause();
}

void MainWindow::resumeResolve()
{
    appendLog(tr("Resuming..."));
    resolver->resume();
}

void MainWindow::stopResolve()
{
    appendLog(tr("Stopping..."));
    resolver->stop();
}

void MainWindow::runBatch()
{
    BatchJob job;
    job.imagePath = QFileDialog::getOpenFileName(this, tr("Select image"), QString(), tr("Images (*.png *.jpg *.jpeg)"));
    if (job.imagePath.isEmpty())
        return;
    job.audioPath = QFileDialog::getOpenFileName(this, tr("Select audio"), QString(), tr("Audio Files (*.wav *.mp3)"));
    if (job.audioPath.isEmpty())
        return;

    QString baseName = QFileInfo(job.imagePath).completeBaseName();
    QString outFile = QDir(logsDir()).filePath(baseName + "_output.mp4");
    job.outputPath = outFile;

    batch->enqueue(job);
    batch->start();
}

void MainWindow::appendLog(const QString &line)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    logView->appendPlainText(QString("[%1] %2").arg(time, line));
}

void MainWindow::updateProgress(int percent)
{
    progress->setValue(percent);
}

void MainWindow::showCompiledResult(const QString &path)
{
    appendLog(tr("Successfully compiled: %1").arg(path));
    statusBar()->showMessage(tr("Compiled: %1").arg(path), 5000);
}

void MainWindow::exitApp()
{
    close();
}
