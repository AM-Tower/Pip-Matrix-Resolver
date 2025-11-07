/****************************************************************
 * @file MainWindow.cpp
 * @brief Implements the main application window.
 *
 * @author Jeffrey Scott Flesher with Copilot
 * @version 0.7
 * @date    2025-11-03
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Implements the main window logic: settings, history, menus,
 * file and URL loaders, logging, and UI dialog wiring.
 * Now with dynamic UI setup instead of .ui file.
 * In Work:
 *  two venvs (main + test), you can extend TerminalEngine with a testVenvPath and a helper like activateTestVenv() so you can switch contexts cleanly.
 *  Would you like me to sketch that extension now
 *  queues a status bar message and mirrors it into the console
 *  fixed version of runNextBatchCommand() with the same correction, so batch execution doesn’t trigger that warning either
 ***************************************************************/

#include "MainWindow.h"
#include <QApplication>
#include <QDebug>
#include <QEventLoop>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QProcess>
#include <QSysInfo>
#include <QUrl>
#include <utility>
#include "Config.h"

#define SHOW_DEBUG 1

/****************************************************************
 * @brief Globals for MainWindow.
 ***************************************************************/
QString MainWindow::appVersion = "1.0"; // Change in main
const QString DEFAULT_PYTHON_VERSION = "3.10";
const QString DEFAULT_PIP_VERSION = "23.2";
const QString DEFAULT_PIPTOOLS_VERSION = "6.13";
const int DEFAULT_MAX_ITEMS = 10;
const QString DEFAULT_APP_VERSION = "1.0";
const QString MainWindow::kOrganizationName = "AM-Tower";
const QString MainWindow::kApplicationName = "PipMatrixResolver";

/****************************************************************
 * @brief Constructor for MainWindow.
 ***************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , requirementsModel(new QStandardItemModel(this))
    , localHistoryModel(new QStandardItemModel(this))
    , webHistoryModel(new QStandardItemModel(this))
    , maxHistoryItems(10)
    , terminalEngine(new TerminalEngine(this))
{
    setupUi();
    // Disable terminal tab at startup
    tabTerminal->setEnabled(false);

    // Check if venv exists
    if (terminalEngine->venvExists())
    {
        // Activate venv by pointing engine at venv’s Python
        QString venvPython = terminalEngine->venvPythonPath(terminalEngine->venvPath);
        terminalEngine->setPythonCommand(venvPython);

        // Enable terminal tab
        tabTerminal->setEnabled(true);
        queueStatusMessage(tr("✅ Virtual environment detected and activated."), 5000);
    }
    else
    {
        queueStatusMessage(tr("⚠️ No virtual environment found. Use Tools → Create venv."), 5000);
    }

    loadAppSettings();
    loadHistory();
    // Statusbar que.
    connect(&statusTimer, &QTimer::timeout, this, &MainWindow::showNextStatusMessage);
    connect(commandsTab, &CommandsTab::requestStatusMessage,
            this, [this](const QString& msg, int timeoutMs){
                statusBar->showMessage(msg, timeoutMs);
            });
    // Connect actions
    connect(actionOpenRequirements, &QAction::triggered, this, &MainWindow::openLocalRequirements);
    connect(actionFetchRequirements,
            &QAction::triggered,
            this,
            &MainWindow::fetchRequirementsFromUrl);
    connect(actionExit, &QAction::triggered, this, &MainWindow::exitApp);
    connect(actionAbout, &QAction::triggered, this, &MainWindow::showAboutBox);
    connect(actionViewReadme, &QAction::triggered, this, &MainWindow::showReadmeDialog);
    connect(actionCreateVenv, &QAction::triggered, this, &MainWindow::onCreateVenv);
    connect(actionResolveMatrix, &QAction::triggered, this, &MainWindow::startResolve);
    connect(actionPause, &QAction::triggered, this, &MainWindow::pauseResolve);
    connect(actionResume, &QAction::triggered, this, &MainWindow::resumeResolve);
    connect(actionStop, &QAction::triggered, this, &MainWindow::stopResolve);

    // Connect settings buttons
    if (buttonBoxPreferences)
    {
        connect(buttonBoxPreferences,
                &QDialogButtonBox::accepted,
                this,
                &MainWindow::saveAppSettings);
        QPushButton *applyBtn = buttonBoxPreferences->button(QDialogButtonBox::Apply);
        if (applyBtn)
        {
            connect(applyBtn, &QPushButton::clicked, this, &MainWindow::saveAppSettings);
        }
    }

    // Connect history table selection changes
    connect(localHistoryTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::updateLocalHistoryButtons);
    connect(webHistoryTable->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::updateWebHistoryButtons);

    // Connect Terminal tab
    connect(runCommandBtn, &QPushButton::clicked, this, &MainWindow::onRunCommand);
    connect(clearTerminalBtn, &QPushButton::clicked, this, &MainWindow::onClearTerminal);
    connect(stopCommandBtn, &QPushButton::clicked, this, &MainWindow::onStopCommand);
    connect(commandInput, &QLineEdit::returnPressed, this, &MainWindow::onRunCommand);

    // Connect Package Manager tab
    connect(searchPackageBtn, &QPushButton::clicked, this, &MainWindow::onSearchPackage);
    connect(installPackageBtn, &QPushButton::clicked, this, &MainWindow::onInstallPackage);
    connect(uninstallPackageBtn, &QPushButton::clicked, this, &MainWindow::onUninstallPackage);

    connect(mainTabs, &QTabWidget::currentChanged, this, [this](int index) {
        if (mainTabs->widget(index)->objectName() == "tabPackageManager")
        {
            refreshInstalledPackages();
        }
    });
    connect(installedPackagesList,
            &QListWidget::doubleClicked,
            this,
            &MainWindow::onInstalledPackagesListDoubleClicked);

    // Refresh tables on startup
    refreshRecentMenus();
    refreshHistoryTables();
    checkAndRestoreSettings();

    detectSystem();
    restoreCpuCudaSettings();
    setupVenvPaths();

}

/****************************************************************
 * @brief Destructor for MainWindow.
 ***************************************************************/
MainWindow::~MainWindow() {}

/****************************************************************
 * @brief Sets up the entire UI dynamically.
 ***************************************************************/
void MainWindow::setupUi()
{
    // Main window setup
    resize(900, 600);
    setWindowTitle(tr("Pip Matrix Resolver"));

    // Central widget
    centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // Tab widget
    mainTabs = new QTabWidget(centralWidget);
    mainTabs->setCurrentIndex(3);
    mainLayout->addWidget(mainTabs);

    // === TAB: MAIN ===
    tabMain = new QWidget();
    tabMain->setObjectName("tabMain");
    QVBoxLayout *mainTabLayout = new QVBoxLayout(tabMain);

    splitter = new QSplitter(Qt::Horizontal, tabMain);
    requirementsView = new QTableView(splitter);
    matrixView = new QTableView(splitter);
    splitter->addWidget(requirementsView);
    splitter->addWidget(matrixView);
    mainTabLayout->addWidget(splitter);

    bottomSplitter = new QSplitter(Qt::Horizontal, tabMain);
    logView = new QPlainTextEdit(bottomSplitter);
    logView->setReadOnly(true);
    progress = new QProgressBar(bottomSplitter);
    bottomSplitter->addWidget(logView);
    bottomSplitter->addWidget(progress);
    mainTabLayout->addWidget(bottomSplitter);

    requirementsView->setModel(requirementsModel);
    requirementsView->setAlternatingRowColors(true);
    requirementsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    requirementsView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    mainTabs->addTab(tabMain, tr("Main"));

    // === TAB: HISTORY ===
    tabHistory = new QWidget();
    tabHistory->setObjectName("tabHistory");
    QVBoxLayout *historyLayout = new QVBoxLayout(tabHistory);

    localHistoryTable = new QTableView(tabHistory);
    localHistoryTable->setModel(localHistoryModel);
    historyLayout->addWidget(localHistoryTable);

    QHBoxLayout *localHistoryButtonBar = new QHBoxLayout();
    localAddButton = new QPushButton(tr("Add"), tabHistory);
    localEditButton = new QPushButton(tr("Edit"), tabHistory);
    localDeleteButton = new QPushButton(tr("Delete"), tabHistory);
    localUpButton = new QPushButton(tr("Up"), tabHistory);
    localDownButton = new QPushButton(tr("Down"), tabHistory);
    localHistoryButtonBar->addWidget(localAddButton);
    localHistoryButtonBar->addWidget(localEditButton);
    localHistoryButtonBar->addWidget(localDeleteButton);
    localHistoryButtonBar->addWidget(localUpButton);
    localHistoryButtonBar->addWidget(localDownButton);
    historyLayout->addLayout(localHistoryButtonBar);

    connect(localAddButton, &QPushButton::clicked, this, &MainWindow::on_localAddButton_clicked);
    connect(localEditButton, &QPushButton::clicked, this, &MainWindow::on_localEditButton_clicked);
    connect(localDeleteButton,
            &QPushButton::clicked,
            this,
            &MainWindow::on_localDeleteButton_clicked);
    connect(localUpButton, &QPushButton::clicked, this, &MainWindow::on_localUpButton_clicked);
    connect(localDownButton, &QPushButton::clicked, this, &MainWindow::on_localDownButton_clicked);

    webHistoryTable = new QTableView(tabHistory);
    webHistoryTable->setModel(webHistoryModel);
    historyLayout->addWidget(webHistoryTable);

    QHBoxLayout *webHistoryButtonBar = new QHBoxLayout();
    webAddButton = new QPushButton(tr("Add"), tabHistory);
    webEditButton = new QPushButton(tr("Edit"), tabHistory);
    webDeleteButton = new QPushButton(tr("Delete"), tabHistory);
    webUpButton = new QPushButton(tr("Up"), tabHistory);
    webDownButton = new QPushButton(tr("Down"), tabHistory);
    webHistoryButtonBar->addWidget(webAddButton);
    webHistoryButtonBar->addWidget(webEditButton);
    webHistoryButtonBar->addWidget(webDeleteButton);
    webHistoryButtonBar->addWidget(webUpButton);
    webHistoryButtonBar->addWidget(webDownButton);
    historyLayout->addLayout(webHistoryButtonBar);

    connect(webAddButton, &QPushButton::clicked, this, &MainWindow::on_webAddButton_clicked);
    connect(webEditButton, &QPushButton::clicked, this, &MainWindow::on_webEditButton_clicked);
    connect(webDeleteButton, &QPushButton::clicked, this, &MainWindow::on_webDeleteButton_clicked);
    connect(webUpButton, &QPushButton::clicked, this, &MainWindow::on_webUpButton_clicked);
    connect(webDownButton, &QPushButton::clicked, this, &MainWindow::on_webDownButton_clicked);

    mainTabs->addTab(tabHistory, tr("History"));

    // === TAB: TERMINAL ===
    tabTerminal = new QWidget();
    tabTerminal->setObjectName("tabTerminal");
    QVBoxLayout *terminalLayout = new QVBoxLayout(tabTerminal);

    terminalOutput = new QPlainTextEdit(tabTerminal);
    terminalLayout->addWidget(terminalOutput);

    QHBoxLayout *terminalCommandLayout = new QHBoxLayout();
    commandInput = new QLineEdit(tabTerminal);
    commandInput->setPlaceholderText("Enter command...");
    runCommandBtn = new QPushButton(tr("Run"), tabTerminal);
    clearTerminalBtn = new QPushButton(tr("Clear"), tabTerminal);
    stopCommandBtn = new QPushButton(tr("Stop"), tabTerminal);
    stopCommandBtn->setEnabled(false);
    terminalCommandLayout->addWidget(commandInput);
    terminalCommandLayout->addWidget(runCommandBtn);
    terminalCommandLayout->addWidget(stopCommandBtn);
    terminalCommandLayout->addWidget(clearTerminalBtn);
    terminalLayout->addLayout(terminalCommandLayout);

    mainTabs->addTab(tabTerminal, tr("Terminal"));

    // === TAB: PACKAGE MANAGER ===
    tabPackageManager = new QWidget();
    tabPackageManager->setObjectName("tabPackageManager");
    QVBoxLayout *packageManagerLayout = new QVBoxLayout(tabPackageManager);

    QHBoxLayout *packageManagerCommandLayout = new QHBoxLayout();
    packageNameInput = new QLineEdit(tabPackageManager);
    searchPackageBtn = new QPushButton(tr("Search"), tabPackageManager);
    installPackageBtn = new QPushButton(tr("Install"), tabPackageManager);
    uninstallPackageBtn = new QPushButton(tr("Uninstall"), tabPackageManager);
    packageManagerCommandLayout->addWidget(packageNameInput);
    packageManagerCommandLayout->addWidget(searchPackageBtn);
    packageManagerCommandLayout->addWidget(installPackageBtn);
    packageManagerCommandLayout->addWidget(uninstallPackageBtn);
    packageManagerLayout->addLayout(packageManagerCommandLayout);

    installedPackagesList = new QListWidget(tabPackageManager);
    packageManagerLayout->addWidget(installedPackagesList);

    packageOutput = new QPlainTextEdit(tabPackageManager);
    packageManagerLayout->addWidget(packageOutput);

    mainTabs->addTab(tabPackageManager, tr("Package Manager"));

    // === TAB: COMMANDS ===
    tabCommands = new QWidget();
    tabCommands->setObjectName("tabCommands");
    QVBoxLayout *commandsLayout = new QVBoxLayout(tabCommands);

    commandsTab = new CommandsTab(terminalEngine, tabCommands);
    commandsLayout->addWidget(commandsTab);

    mainTabs->addTab(tabCommands, tr("Commands"));

    // === TAB: SETTINGS ===
    tabSettings = new QWidget();
    tabSettings->setObjectName("tabSettings");
    QVBoxLayout *settingsLayout = new QVBoxLayout(tabSettings);

    QFormLayout *formLayout = new QFormLayout();

    pythonVersionEdit = new QLineEdit(tabSettings);
    formLayout->addRow(tr("Python version:"), pythonVersionEdit);

    pipVersionEdit = new QLineEdit(tabSettings);
    formLayout->addRow(tr("pip version:"), pipVersionEdit);

    pipToolsVersionEdit = new QLineEdit(tabSettings);
    formLayout->addRow(tr("pip-tools version:"), pipToolsVersionEdit);

    spinMaxItems = new QSpinBox(tabSettings);
    spinMaxItems->setMinimum(-1);
    spinMaxItems->setMaximum(std::numeric_limits<int>::max());
    spinMaxItems->setValue(10);
    spinMaxItems->setToolTip(tr("-1 = unlimited, 0 not allowed, ≥1 valid"));
    formLayout->addRow(tr("Maximum number of items:"), spinMaxItems);

    gpuDetectedCheckBox = new QCheckBox(tabSettings);
    gpuDetectedCheckBox->setEnabled(false);
    formLayout->addRow(tr("GPU Detected:"), gpuDetectedCheckBox);

    useCpuCheckBox = new QCheckBox(tabSettings);
    formLayout->addRow(tr("Use CPU:"), useCpuCheckBox);

    cudaCheckBox = new QCheckBox(tabSettings);
    formLayout->addRow(tr("Cuda:"), cudaCheckBox);

    osEdit = new QLineEdit(tabSettings);
    osEdit->setReadOnly(true);
    formLayout->addRow(tr("OS:"), osEdit);

    osReleaseEdit = new QLineEdit(tabSettings);
    osReleaseEdit->setReadOnly(true);
    formLayout->addRow(tr("Release:"), osReleaseEdit);

    osVersionEdit = new QLineEdit(tabSettings);
    osVersionEdit->setReadOnly(true);
    formLayout->addRow(tr("Version:"), osVersionEdit);

    settingsLayout->addLayout(formLayout);

    // Buttons and button box
    saveSettingsButton = new QPushButton(tr("Save Settings"), tabSettings);
    settingsLayout->addWidget(saveSettingsButton);

    restoreDefaultsButton = new QPushButton(tr("Restore Defaults"), tabSettings);
    settingsLayout->addWidget(restoreDefaultsButton);

    // Create the button box BEFORE connecting any signals
    buttonBoxPreferences = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply
                                                    | QDialogButtonBox::Cancel,
                                                tabSettings);
    settingsLayout->addWidget(buttonBoxPreferences);

    mainTabs->addTab(tabSettings, tr("Settings"));

    // === MENU BAR ===
    menuBar = new QMenuBar(this);
    setMenuBar(menuBar);

    // File menu
    menuFile = new QMenu(tr("&File"), this);
    menuBar->addMenu(menuFile);

    actionOpenRequirements = new QAction(QIcon(":/icons/icons/open.svg"),
                                         tr("Open requirements file..."),
                                         this);
    menuFile->addAction(actionOpenRequirements);

    actionFetchRequirements = new QAction(QIcon(":/icons/icons/url.svg"),
                                          tr("Fetch requirements from URL..."),
                                          this);
    menuFile->addAction(actionFetchRequirements);

    menuFile->addSeparator();

    // Recent menus (created here, populated later)
    recentLocalMenu = new QMenu(tr("Recent Local"), this);
    menuFile->addMenu(recentLocalMenu);

    recentWebMenu = new QMenu(tr("Recent Web"), this);
    menuFile->addMenu(recentWebMenu);

    menuFile->addSeparator();

    actionExit = new QAction(tr("Exit"), this);
    menuFile->addAction(actionExit);

    // Tools menu
    menuTools = new QMenu(tr("&Tools"), this);
    menuBar->addMenu(menuTools);

    actionCreateVenv = new QAction(QIcon(":/icons/icons/venv.svg"), tr("Create venv"), this);
    menuTools->addAction(actionCreateVenv);

    actionResolveMatrix = new QAction(QIcon(":/icons/icons/resolve.svg"),
                                      tr("Resolve matrix"),
                                      this);
    menuTools->addAction(actionResolveMatrix);

    actionPause = new QAction(QIcon(":/icons/icons/pause.svg"), tr("Pause"), this);
    menuTools->addAction(actionPause);

    actionResume = new QAction(QIcon(":/icons/icons/resume.svg"), tr("Resume"), this);
    menuTools->addAction(actionResume);

    actionStop = new QAction(QIcon(":/icons/icons/stop.svg"), tr("Stop"), this);
    menuTools->addAction(actionStop);

    // Batch menu
    menuBatch = new QMenu(tr("&Batch"), this);
    menuBar->addMenu(menuBatch);

    actionRunBatch = new QAction(QIcon(":/icons/icons/batch.svg"), tr("Run batch"), this);
    menuBatch->addAction(actionRunBatch);

    // Help menu
    menuHelp = new QMenu(tr("&Help"), this);
    menuBar->addMenu(menuHelp);

    actionAbout = new QAction(QIcon(":/icons/icons/info.svg"), tr("About"), this);
    menuHelp->addAction(actionAbout);

    actionViewReadme = new QAction(QIcon(":/icons/icons/readme.svg"), tr("View README"), this);
    menuHelp->addAction(actionViewReadme);

    // === TOOLBAR ===
    mainToolBar = new QToolBar(tr("Main Toolbar"), this);
    addToolBar(Qt::TopToolBarArea, mainToolBar);

    mainToolBar->addAction(actionOpenRequirements);
    mainToolBar->addAction(actionFetchRequirements);
    mainToolBar->addAction(actionCreateVenv);
    mainToolBar->addAction(actionResolveMatrix);
    mainToolBar->addAction(actionPause);
    mainToolBar->addAction(actionResume);
    mainToolBar->addAction(actionStop);
    mainToolBar->addAction(actionRunBatch);
    mainToolBar->addAction(actionAbout);
    mainToolBar->addAction(actionViewReadme);

    // === STATUS BAR ===
    statusBar = new QStatusBar(this);
    setStatusBar(statusBar);

    // === Terminal Engine wiring and Terminal tab gating ===
    terminalEngine = new TerminalEngine(this);

    // Connect TerminalEngine signals
    connect(terminalEngine, &TerminalEngine::outputReceived, this, &MainWindow::onTerminalOutput);
    connect(terminalEngine,
            &TerminalEngine::commandStarted,
            this,
            &MainWindow::onTerminalCommandStarted);
    connect(terminalEngine,
            &TerminalEngine::commandFinished,
            this,
            &MainWindow::onTerminalCommandFinished);
    connect(terminalEngine, &TerminalEngine::venvProgress, this, &MainWindow::onVenvProgress);

    // Settings tab connections (safe order)
    connect(saveSettingsButton, &QPushButton::clicked, this, &MainWindow::onSaveSettings);
    connect(restoreDefaultsButton, &QPushButton::clicked, this, &MainWindow::onRestoreDefaults);
    connect(buttonBoxPreferences, &QDialogButtonBox::accepted, this, &MainWindow::saveAppSettings);
    if (QPushButton *applyBtn = buttonBoxPreferences->button(QDialogButtonBox::Apply))
    {
        connect(applyBtn, &QPushButton::clicked, this, &MainWindow::onApplySettings);
    }

    // After creating terminalEngine and before venv checks:
    QString versionFromSettings = pythonVersionEdit->text().trimmed();
    terminalEngine->setPythonCommand(versionFromSettings);

    // Default venv path to project-local ".venv"
    terminalEngine->setVenvPath(QDir::currentPath() + "/.venv");

    // Disable Terminal tab at startup until venv is available and activated
    int terminalTabIndex = mainTabs->indexOf(tabTerminal);
    if (terminalTabIndex >= 0)
    {
        mainTabs->setTabEnabled(terminalTabIndex, false);
    }

    // Attempt to enable if venv exists and activation succeeds
    if (terminalEngine->venvExists())
    {
        if (terminalEngine->activateVenv())
        {
            if (terminalTabIndex >= 0)
            {
                mainTabs->setTabEnabled(terminalTabIndex, true);
            }
            queueStatusMessage(tr("Virtual environment detected and activated"), 5000);
        }
        else
        {
            queueStatusMessage(tr("Virtual environment present but activation failed"), 5000);
        }
    }
    else
    {
        queueStatusMessage(tr("No virtual environment found. Use Tools → Create venv."), 5000);
    }
}

/****************************************************************
 * @brief Exits the application.
 ***************************************************************/
void MainWindow::exitApp()
{
    QApplication::quit();
}

/****************************************************************
 * @brief Opens requirements from a local file via dialog.
 ***************************************************************/
void MainWindow::openLocalRequirements()
{
    QString path = QFileDialog::getOpenFileName(this,
                                                tr("Open requirements.txt"),
                                                QString(),
                                                tr("Text Files (*.txt)"));
    if (path.isEmpty())
    {
        return;
    }
    loadRequirementsFromFile(path);
}

/****************************************************************
 * @brief Fetches requirements from a URL via dialog.
 ***************************************************************/
void MainWindow::fetchRequirementsFromUrl()
{
    bool ok = false;
    QString inputUrl = QInputDialog::getText(this,
                                             tr("Fetch requirements"),
                                             tr("Enter URL:"),
                                             QLineEdit::Normal,
                                             "",
                                             &ok);
    if (!ok || inputUrl.isEmpty())
    {
        return;
    }
    const QString rawUrl = normalizeRawUrl(inputUrl);
    loadRequirementsFromUrl(rawUrl);
}

/****************************************************************
 * @brief Loads requirements from a local file.
 ***************************************************************/
void MainWindow::loadRequirementsFromFile(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }
    if (!QFile::exists(path))
    {
        QMessageBox::warning(this, tr("File missing"), tr("File no longer exists:\n%1").arg(path));
        historyRecentLocal.removeAll(path);
        refreshRecentMenus();
        saveHistory();
        return;
    }
    const QStringList lines = readTextFileLines(path);
    QStringList errors;
    if (!validateRequirementsWithErrors(lines, errors))
    {
        QMessageBox::warning(this,
                             tr("Invalid requirements.txt"),
                             tr("Validation failed:\n%1").arg(errors.join("\n")));
        return;
    }
    if (requirementsModel)
    {
        requirementsModel->clear();
        writeTableToModel(lines);
    }
    applySettingsFromUi();
    historyRecentLocal.removeAll(path);
    historyRecentLocal.prepend(path);
    if (maxHistoryItems != -1)
    {
        while (historyRecentLocal.size() > maxHistoryItems)
        {
            historyRecentLocal.removeLast();
        }
    }
    refreshRecentMenus();
    saveHistory();
    appendLog(tr("Loaded %1 requirements from %2").arg(requirementsModel->rowCount()).arg(path));
}

/****************************************************************
 * @brief Loads requirements from a URL.
 ***************************************************************/
void MainWindow::loadRequirementsFromUrl(const QString &url)
{
    if (url.isEmpty())
    {
        return;
    }
    QByteArray content;
    if (!downloadText(url, content))
    {
        QMessageBox::warning(this,
                             tr("Download failed"),
                             tr("Failed to fetch requirements from URL:\n%1").arg(url));
        historyRecentWeb.removeAll(url);
        refreshRecentMenus();
        saveHistory();
        return;
    }
    const QStringList lines = QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);
    QStringList errors;
    if (!validateRequirementsWithErrors(lines, errors))
    {
        QMessageBox::warning(this,
                             tr("Invalid requirements.txt"),
                             tr("Fetched content failed validation:\n%1").arg(errors.join("\n")));
        return;
    }
    if (requirementsModel)
    {
        requirementsModel->clear();
        writeTableToModel(lines);
    }
    applySettingsFromUi();
    historyRecentWeb.removeAll(url);
    historyRecentWeb.prepend(url);
    if (maxHistoryItems != -1)
    {
        while (historyRecentWeb.size() > maxHistoryItems)
        {
            historyRecentWeb.removeLast();
        }
    }
    refreshRecentMenus();
    saveHistory();
    appendLog(
        tr("Fetched %1 requirements from URL: %2").arg(requirementsModel->rowCount()).arg(url));
}

/****************************************************************
 * @brief Populates the local and web history tables in the
 *        History tab with the current history data from QSettings.
 ***************************************************************/
void MainWindow::refreshHistoryTables()
{
    QSettings s(kOrganizationName, kApplicationName);
    QStringList localList = s.value("history/recentLocal").toStringList();
    QStringList webList = s.value("history/recentWeb").toStringList();

    localHistoryModel->clear();
    localHistoryModel->setHorizontalHeaderLabels({tr("Recent Local Files")});
    for (const QString &path : std::as_const(localList))
    {
        QList<QStandardItem *> row;
        row << new QStandardItem(path);
        localHistoryModel->appendRow(row);
    }
    localHistoryTable->resizeColumnsToContents();

    webHistoryModel->clear();
    webHistoryModel->setHorizontalHeaderLabels({tr("Recent Web URLs")});
    for (const QString &url : std::as_const(webList))
    {
        QList<QStandardItem *> row;
        row << new QStandardItem(url);
        webHistoryModel->appendRow(row);
    }
    webHistoryTable->resizeColumnsToContents();
    updateLocalHistoryButtons();
    updateWebHistoryButtons();
}

/****************************************************************
 * @brief Refreshes recent file and URL menus safely.
 ***************************************************************/
void MainWindow::refreshRecentMenus()
{
    if (!recentLocalMenu || !recentWebMenu)
    {
        qWarning() << "refreshRecentMenus: menus not available, skipping";
        return;
    }
    recentLocalMenu->clear();
    recentWebMenu->clear();

    // Populate Recent Local
    for (int i = 0; i < historyRecentLocal.size(); ++i)
    {
        const QString &path = historyRecentLocal.at(i);
        QAction *act = recentLocalMenu->addAction(path);
        connect(act, &QAction::triggered, this, [this, path]() { loadRequirementsFromFile(path); });
    }
    if (!historyRecentLocal.isEmpty())
    {
        recentLocalMenu->addSeparator();
        QAction *clearLocal = recentLocalMenu->addAction(tr("Clear Local History"));
        connect(clearLocal, &QAction::triggered, this, [this]() {
            historyRecentLocal.clear();
            refreshRecentMenus();
            saveHistory();
        });
    }

    // Populate Recent Web
    for (int i = 0; i < historyRecentWeb.size(); ++i)
    {
        const QString &url = historyRecentWeb.at(i);
        QAction *act = recentWebMenu->addAction(url);
        connect(act, &QAction::triggered, this, [this, url]() { loadRequirementsFromUrl(url); });
    }
    if (!historyRecentWeb.isEmpty())
    {
        recentWebMenu->addSeparator();
        QAction *clearWeb = recentWebMenu->addAction(tr("Clear Web History"));
        connect(clearWeb, &QAction::triggered, this, [this]() {
            historyRecentWeb.clear();
            refreshRecentMenus();
            saveHistory();
        });
    }
}

/****************************************************************
 * @brief Clears both local and web recent history lists.
 ***************************************************************/
void MainWindow::clearAllHistory()
{
    historyRecentLocal.clear();
    historyRecentWeb.clear();
    refreshRecentMenus();
    saveHistory();
    appendLog(tr("Cleared all history"));
}

/****************************************************************
 * @brief Loads persisted application + Settings values at startup.
 ***************************************************************/
void MainWindow::loadAppSettings()
{
    QSettings settings;

    // Load values with defaults
    QString pythonVer = settings.value("PythonVersion", DEFAULT_PYTHON_VERSION).toString();
    QString pipVer = settings.value("PipVersion", DEFAULT_PIP_VERSION).toString();
    QString pipToolsVer = settings.value("PipToolsVersion", DEFAULT_PIPTOOLS_VERSION).toString();
    int maxItems = settings.value("app/maxItems", DEFAULT_MAX_ITEMS).toInt();

    // Update internal state
    maxHistoryItems = maxItems;

    // Update UI fields
    pythonVersionEdit->setText(pythonVer);
    pipVersionEdit->setText(pipVer);
    pipToolsVersionEdit->setText(pipToolsVer);
    spinMaxItems->setValue(maxItems);

    // Apply Python command immediately
    terminalEngine->setPythonCommand(pythonVer);

    // Validate and sync UI
    validateAppSettings();
    updateUiFromSettings();
}

/****************************************************************
 * @brief Save Settings button handler.
 ***************************************************************/
void MainWindow::onSaveSettings()
{
    QString pythonVer = pythonVersionEdit->text().trimmed();
    QString pipVer = pipVersionEdit->text().trimmed();
    QString pipToolsVer = pipToolsVersionEdit->text().trimmed();
    int maxItems = spinMaxItems->value();

    terminalEngine->setPythonCommand(pythonVer);

    QSettings settings;
    settings.setValue("PythonVersion", pythonVer);
    settings.setValue("PipVersion", pipVer);
    settings.setValue("PipToolsVersion", pipToolsVer);
    settings.setValue("app/maxItems", maxItems);
    settings.sync();

    queueStatusMessage(tr("Settings saved. Python command updated to: %1").arg(terminalEngine->pythonCommand()), 5000);
}

/****************************************************************
 * @brief Apply Settings button handler.
 ***************************************************************/
void MainWindow::onApplySettings()
{
    QString pythonVer = pythonVersionEdit->text().trimmed();
    terminalEngine->setPythonCommand(pythonVer);

    queueStatusMessage(tr("Settings applied. Python command updated to: %1").arg(terminalEngine->pythonCommand()), 5000);
}

/****************************************************************
 * @brief Restore Defaults button handler.
 ***************************************************************/
void MainWindow::onRestoreDefaults()
{
    // Reset UI fields using constants
    pythonVersionEdit->setText(DEFAULT_PYTHON_VERSION);
    pipVersionEdit->setText(DEFAULT_PIP_VERSION);
    pipToolsVersionEdit->setText(DEFAULT_PIPTOOLS_VERSION);
    spinMaxItems->setValue(DEFAULT_MAX_ITEMS);
    useCpuCheckBox->setChecked(false);
    cudaCheckBox->setChecked(false);

    // Update runtime Python command using default
    terminalEngine->setPythonCommand(DEFAULT_PYTHON_VERSION);

    // Persist defaults
    QSettings settings;
    settings.setValue("PythonVersion", DEFAULT_PYTHON_VERSION);
    settings.setValue("PipVersion", DEFAULT_PIP_VERSION);
    settings.setValue("PipToolsVersion", DEFAULT_PIPTOOLS_VERSION);
    settings.setValue("app/maxItems", DEFAULT_MAX_ITEMS);
    settings.setValue("AppVersion", DEFAULT_APP_VERSION);
    settings.sync();

    queueStatusMessage(tr("Defaults restored. Python command set to: %1").arg(terminalEngine->pythonCommand()), 5000);
}

/****************************************************************
 * @brief Save Settings when dialog accepted (OK).
 ***************************************************************/
void MainWindow::saveAppSettings()
{
    QString pythonVer = pythonVersionEdit->text().trimmed();
    QString pipVer = pipVersionEdit->text().trimmed();
    QString pipToolsVer = pipToolsVersionEdit->text().trimmed();
    int maxItems = spinMaxItems->value();

    terminalEngine->setPythonCommand(pythonVer);

    QSettings settings;
    settings.setValue("PythonVersion", pythonVer);
    settings.setValue("PipVersion", pipVer);
    settings.setValue("PipToolsVersion", pipToolsVer);
    settings.setValue("app/maxItems", maxItems);
    settings.setValue("AppVersion", DEFAULT_APP_VERSION);
    settings.sync();

    queueStatusMessage(tr("Application settings saved. Python command updated to: %1").arg(terminalEngine->pythonCommand()), 5000);
}

/****************************************************************
 * @brief Validates and clamps settings to safe ranges.
 ***************************************************************/
void MainWindow::validateAppSettings()
{
    if (maxHistoryItems == 0)
    {
        maxHistoryItems = 10;
    }
    else if (maxHistoryItems < -1)
    {
        maxHistoryItems = 1;
    }
}

/****************************************************************
 * @brief Applies settings from UI widgets to member state.
 ***************************************************************/
void MainWindow::applySettingsFromUi()
{
    maxHistoryItems = spinMaxItems->value();
}

/****************************************************************
 * @brief Updates UI widgets to reflect current settings.
 ***************************************************************/
void MainWindow::updateUiFromSettings()
{
    spinMaxItems->setValue(maxHistoryItems);
}

/****************************************************************
 * @brief Writes requirements into a one-column model.
 ***************************************************************/
void MainWindow::writeTableToModel(const QStringList &lines)
{
    if (!requirementsModel || !requirementsView)
    {
        return;
    }
    requirementsModel->clear();
    requirementsModel->setColumnCount(1);
    requirementsModel->setHorizontalHeaderLabels({tr("requirements.txt")});
    requirementsView->resizeRowsToContents();

    for (const QString &line : lines)
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
        {
            continue;
        }
        auto *item = new QStandardItem(trimmed);
        item->setEditable(false);
        requirementsModel->appendRow(item);
    }
    requirementsView->resizeColumnsToContents();
    requirementsView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    requirementsView->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    requirementsView->resizeColumnsToContents();
    requirementsView->resizeRowsToContents();

    // Get the actual width of requirementsView and add 66px
    int reqWidth = requirementsView->verticalHeader()->width();
    for (int col = 0; col < requirementsModel->columnCount(); ++col)
    {
        reqWidth += requirementsView->columnWidth(col);
    }
    reqWidth += 66;

    // Set the splitter sizes
    if (splitter)
    {
        QList<int> sizes;
        sizes << reqWidth << qMax(100, splitter->width() - reqWidth);
        splitter->setSizes(sizes);
    }
    refreshHistoryTables();
}

/****************************************************************
 * @brief Appends a line to the log view with timestamp.
 * @param line The message line to append.
 ***************************************************************/
void MainWindow::appendLog(const QString &line)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    logView->appendPlainText(QString("[%1] %2").arg(time, line));
}

/****************************************************************
 * @brief Updates progress bar percent.
 * @param percent The progress value (0-100).
 ***************************************************************/
void MainWindow::updateProgress(int percent)
{
    progress->setValue(percent);
}

/****************************************************************
 * @brief Shows compiled result message path.
 * @param path The compiled output path.
 ***************************************************************/
void MainWindow::showCompiledResult(const QString &path)
{
    appendLog(tr("Successfully compiled: %1").arg(path));
    queueStatusMessage(tr("Compiled successfully"), 5000);
}

/****************************************************************
 * @brief Shows About dialog using current appVersion.
 ***************************************************************/
void MainWindow::showAboutBox()
{
    QMessageBox::about(
        this,
        tr("About Pip Matrix Resolver"),
        tr("<b>Pip Matrix Resolver</b><br>" "Cross-platform Qt tool to resolve " "Python dependency matrices.<br>" "Version %1")
            .arg(appVersion));
}

/****************************************************************
 * @brief Shows README dialog from resources.
 ***************************************************************/
void MainWindow::showReadmeDialog()
{
    QFile file(":/docs/README.md");
    QString markdown;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        markdown = QString::fromUtf8(file.readAll());
    }
    else
    {
        markdown = tr("README.md not found in resources.");
    }

    QDialog dialog(this);
    dialog.setWindowTitle(tr("README"));
    dialog.resize(700, 500);

    QVBoxLayout layout(&dialog);
    QTextBrowser viewer(&dialog);
    viewer.setMarkdown(markdown);
    viewer.setOpenExternalLinks(true);

    QPushButton closeButton(tr("Close"), &dialog);
    layout.addWidget(&viewer);
    layout.addWidget(&closeButton);

    connect(&closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

/****************************************************************
 * @brief Loads persistent history from QSettings.
 ***************************************************************/
void MainWindow::loadHistory()
{
    QSettings s(kOrganizationName, kApplicationName);
    historyRecentLocal = s.value("history/recentLocal").toStringList();
    historyRecentWeb = s.value("history/recentWeb").toStringList();

    if (maxHistoryItems != -1)
    {
        while (historyRecentLocal.size() > maxHistoryItems)
        {
            historyRecentLocal.removeLast();
        }
        while (historyRecentWeb.size() > maxHistoryItems)
        {
            historyRecentWeb.removeLast();
        }
    }
}

/****************************************************************
 * @brief Saves persistent history to QSettings.
 ***************************************************************/
void MainWindow::saveHistory()
{
    QSettings s(kOrganizationName, kApplicationName);
    s.setValue("history/recentLocal", historyRecentLocal);
    s.setValue("history/recentWeb", historyRecentWeb);
}

/****************************************************************
 * @brief Normalizes a raw URL string.
 ***************************************************************/
QString MainWindow::normalizeRawUrl(const QString &inputUrl)
{
    QString url = inputUrl.trimmed();
    if (!url.startsWith("http://") && !url.startsWith("https://"))
    {
        url.prepend("https://");
    }
    return url;
}

/****************************************************************
 * @brief Reads all lines from a text file.
 ***************************************************************/
QStringList MainWindow::readTextFileLines(const QString &path)
{
    QStringList lines;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!file.atEnd())
        {
            lines << QString::fromUtf8(file.readLine()).trimmed();
        }
        file.close();
    }
    return lines;
}

/****************************************************************
 * @brief Validates requirements.txt lines.
 ***************************************************************/
bool MainWindow::validateRequirementsWithErrors(const QStringList &lines, QStringList &errors)
{
    errors.clear();
    bool valid = true;
    for (const QString &line : lines)
    {
        if (line.isEmpty())
        {
            continue;
        }
        // Example validation: must not start with a dash or space
        if (line.startsWith('-') || line.startsWith(' '))
        {
            errors << tr("Invalid line: %1").arg(line);
            valid = false;
        }
        // Add more validation rules as needed
    }
    return valid;
}

/****************************************************************
 * @brief Downloads text from a URL.
 ***************************************************************/
bool MainWindow::downloadText(const QString &url, QByteArray &out)
{
    QNetworkAccessManager manager;
    QNetworkRequest request{QUrl(url)};
    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() != QNetworkReply::NoError)
    {
        reply->deleteLater();
        return false;
    }
    out = reply->readAll();
    reply->deleteLater();
    return true;
}

/****************************************************************
 * @brief Returns the logs directory path.
 ***************************************************************/
QString MainWindow::logsDir()
{
    QString dir = QDir::homePath() + "/PipMatrixResolverLogs";
    QDir().mkpath(dir);
    return dir;
}

/****************************************************************
 * @brief Starts matrix resolution (stub).
 ***************************************************************/
void MainWindow::startResolve()
{
    appendLog(tr("Starting matrix resolution..."));
    // Add your matrix resolution logic here
}

/****************************************************************
 * @brief Pauses matrix resolution (stub).
 ***************************************************************/
void MainWindow::pauseResolve()
{
    appendLog(tr("Pausing..."));
    // Add your pause logic here
}

/****************************************************************
 * @brief Resumes matrix resolution (stub).
 ***************************************************************/
void MainWindow::resumeResolve()
{
    appendLog(tr("Resuming..."));
    // Add your resume logic here
}

/****************************************************************
 * @brief Stops matrix resolution (stub).
 ***************************************************************/
void MainWindow::stopResolve()
{
    appendLog(tr("Stopping..."));
    // Add your stop logic here
}

/****************************************************************
 * @brief Adds a new entry to the local history table and QSettings.
 ***************************************************************/
void MainWindow::on_localAddButton_clicked()
{
    bool ok;
    QString path = QInputDialog::getText(this,
                                         tr("Add Local File"),
                                         tr("File path:"),
                                         QLineEdit::Normal,
                                         "",
                                         &ok);
    if (ok && !path.isEmpty())
    {
        QSettings s(kOrganizationName, kApplicationName);
        QStringList list = s.value("history/recentLocal").toStringList();
        list.prepend(path);
        s.setValue("history/recentLocal", list);
        refreshHistoryTables();
    }
}

/****************************************************************
 * @brief Edits the selected entry in the local history table and QSettings.
 ***************************************************************/
void MainWindow::on_localEditButton_clicked()
{
    QModelIndex idx = localHistoryTable->currentIndex();
    if (!idx.isValid())
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentLocal").toStringList();
    QString oldValue = list.at(idx.row());
    bool ok;
    QString newValue = QInputDialog::getText(this,
                                             tr("Edit Local File"),
                                             tr("File path:"),
                                             QLineEdit::Normal,
                                             oldValue,
                                             &ok);
    if (!ok)
        return; // User cancelled, do nothing
    if (newValue.isEmpty())
        return; // Optionally, do nothing if empty
    list[idx.row()] = newValue;
    s.setValue("history/recentLocal", list);
    refreshHistoryTables();
}

/****************************************************************
 * @brief Deletes the selected entry from the local history table and QSettings.
 ***************************************************************/
void MainWindow::on_localDeleteButton_clicked()
{
    QModelIndex idx = localHistoryTable->currentIndex();
    if (!idx.isValid())
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentLocal").toStringList();
    list.removeAt(idx.row());
    s.setValue("history/recentLocal", list);
    refreshHistoryTables();
}

/****************************************************************
 * @brief Moves the selected entry up in the local history table and QSettings.
 ***************************************************************/
void MainWindow::on_localUpButton_clicked()
{
    QModelIndex idx = localHistoryTable->currentIndex();
    if (!idx.isValid() || idx.row() == 0)
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentLocal").toStringList();
    list.swapItemsAt(idx.row(), idx.row() - 1);
    s.setValue("history/recentLocal", list);
    refreshHistoryTables();
    localHistoryTable->selectRow(idx.row() - 1);
}

/****************************************************************
 * @brief Moves the selected entry down in the local history table and QSettings.
 ***************************************************************/
void MainWindow::on_localDownButton_clicked()
{
    QModelIndex idx = localHistoryTable->currentIndex();
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentLocal").toStringList();
    if (!idx.isValid() || idx.row() >= list.size() - 1)
        return;
    list.swapItemsAt(idx.row(), idx.row() + 1);
    s.setValue("history/recentLocal", list);
    refreshHistoryTables();
    localHistoryTable->selectRow(idx.row() + 1);
}

/****************************************************************
 * @brief Adds a new entry to the web history table and QSettings.
 ***************************************************************/
void MainWindow::on_webAddButton_clicked()
{
    bool ok;
    QString url
        = QInputDialog::getText(this, tr("Add Web URL"), tr("URL:"), QLineEdit::Normal, "", &ok);
    if (ok && !url.isEmpty())
    {
        QSettings s(kOrganizationName, kApplicationName);
        QStringList list = s.value("history/recentWeb").toStringList();
        list.prepend(url);
        s.setValue("history/recentWeb", list);
        refreshHistoryTables();
    }
}

/****************************************************************
 * @brief Edits the selected entry in the web history table and QSettings.
 ***************************************************************/
void MainWindow::on_webEditButton_clicked()
{
    QModelIndex idx = webHistoryTable->currentIndex();
    if (!idx.isValid())
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentWeb").toStringList();
    QString oldValue = list.at(idx.row());
    bool ok;
    QString newValue = QInputDialog::getText(this,
                                             tr("Edit Web URL"),
                                             tr("URL:"),
                                             QLineEdit::Normal,
                                             oldValue,
                                             &ok);
    if (!ok)
        return; // User cancelled, do nothing
    if (newValue.isEmpty())
        return; // Optionally, do nothing if empty
    list[idx.row()] = newValue;
    s.setValue("history/recentWeb", list);
    refreshHistoryTables();
}

/****************************************************************
 * @brief Deletes the selected entry from the web history table and QSettings.
 ***************************************************************/
void MainWindow::on_webDeleteButton_clicked()
{
    QModelIndex idx = webHistoryTable->currentIndex();
    if (!idx.isValid())
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentWeb").toStringList();
    list.removeAt(idx.row());
    s.setValue("history/recentWeb", list);
    refreshHistoryTables();
}

/****************************************************************
 * @brief Moves the selected entry up in the web history table and QSettings.
 ***************************************************************/
void MainWindow::on_webUpButton_clicked()
{
    QModelIndex idx = webHistoryTable->currentIndex();
    if (!idx.isValid() || idx.row() == 0)
        return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentWeb").toStringList();
    list.swapItemsAt(idx.row(), idx.row() - 1);
    s.setValue("history/recentWeb", list);
    refreshHistoryTables();
    webHistoryTable->selectRow(idx.row() - 1);
}

/****************************************************************
 * @brief Moves the selected entry down in the web history table and QSettings.
 ***************************************************************/
void MainWindow::on_webDownButton_clicked()
{
    QModelIndex idx = webHistoryTable->currentIndex();
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentWeb").toStringList();
    if (!idx.isValid() || idx.row() >= list.size() - 1)
        return;
    list.swapItemsAt(idx.row(), idx.row() + 1);
    s.setValue("history/recentWeb", list);
    refreshHistoryTables();
    webHistoryTable->selectRow(idx.row() + 1);
}

/****************************************************************
 * @brief Enables/disables local history buttons based on selection and table size.
 ***************************************************************/
void MainWindow::updateLocalHistoryButtons()
{
    QItemSelectionModel *sel = localHistoryTable->selectionModel();
    int rowCount = localHistoryModel->rowCount();
    bool hasSelection = sel->hasSelection();

    localEditButton->setEnabled(hasSelection);
    localDeleteButton->setEnabled(hasSelection);

    // Up: enabled if selected and not first row, and more than 1 item
    bool canUp = hasSelection && rowCount > 1;
    if (canUp)
    {
        QModelIndex idx = sel->currentIndex();
        canUp = idx.isValid() && idx.row() > 0;
    }
    localUpButton->setEnabled(canUp);

    // Down: enabled if selected and not last row, and more than 1 item
    bool canDown = hasSelection && rowCount > 1;
    if (canDown)
    {
        QModelIndex idx = sel->currentIndex();
        canDown = idx.isValid() && idx.row() < rowCount - 1;
    }
    localDownButton->setEnabled(canDown);
}

/****************************************************************
 * @brief Enables/disables web history buttons based on selection and table size.
 ***************************************************************/
void MainWindow::updateWebHistoryButtons()
{
    QItemSelectionModel *sel = webHistoryTable->selectionModel();
    int rowCount = webHistoryModel->rowCount();
    bool hasSelection = sel->hasSelection();

    webEditButton->setEnabled(hasSelection);
    webDeleteButton->setEnabled(hasSelection);

    // Up: enabled if selected and not first row, and more than 1 item
    bool canUp = hasSelection && rowCount > 1;
    if (canUp)
    {
        QModelIndex idx = sel->currentIndex();
        canUp = idx.isValid() && idx.row() > 0;
    }
    webUpButton->setEnabled(canUp);

    // Down: enabled if selected and not last row, and more than 1 item
    bool canDown = hasSelection && rowCount > 1;
    if (canDown)
    {
        QModelIndex idx = sel->currentIndex();
        canDown = idx.isValid() && idx.row() < rowCount - 1;
    }
    webDownButton->setEnabled(canDown);
}

/****************************************************************
 * @brief save Settings.
 ***************************************************************/
void MainWindow::saveSettings()
{
    QSettings s(kOrganizationName, kApplicationName);
    s.setValue("settings/pythonVersion", pythonVersionEdit->text());
    s.setValue("settings/pipVersion", pipVersionEdit->text());
    s.setValue("settings/pipToolsVersion", pipToolsVersionEdit->text());
    s.setValue("settings/maxItems", spinMaxItems->value());
    s.setValue("settings/useCpu", useCpuCheckBox->isChecked());
    s.setValue("settings/cuda", cudaCheckBox->isChecked());
}

/****************************************************************
 * @brief check And Restore Settings.
 ***************************************************************/
void MainWindow::checkAndRestoreSettings()
{
    QSettings s(kOrganizationName, kApplicationName);

    // Python version
    QString pythonVersion = s.value("settings/pythonVersion", DEFAULT_PYTHON_VERSION).toString();
    if (pythonVersion.isEmpty())
    {
        pythonVersion = DEFAULT_PYTHON_VERSION;
        s.setValue("settings/pythonVersion", pythonVersion);
    }
    pythonVersionEdit->setText(pythonVersion);

    // pip version
    QString pipVersion = s.value("settings/pipVersion", DEFAULT_PIP_VERSION).toString();
    if (pipVersion.isEmpty())
    {
        pipVersion = DEFAULT_PIP_VERSION;
        s.setValue("settings/pipVersion", pipVersion);
    }
    pipVersionEdit->setText(pipVersion);

    // pip-tools version
    QString pipToolsVersion = s.value("settings/pipToolsVersion", DEFAULT_PIPTOOLS_VERSION)
                                  .toString();
    if (pipToolsVersion.isEmpty())
    {
        pipToolsVersion = DEFAULT_PIPTOOLS_VERSION;
        s.setValue("settings/pipToolsVersion", pipToolsVersion);
    }
    pipToolsVersionEdit->setText(pipToolsVersion);

    // Max items
    int maxItems = s.value("settings/maxItems", DEFAULT_MAX_ITEMS).toInt();
    if (maxItems == 0)
    {
        maxItems = DEFAULT_MAX_ITEMS;
        s.setValue("settings/maxItems", maxItems);
    }
    spinMaxItems->setValue(maxItems);
}

/****************************************************************
 * @brief Restores CPU and Cuda settings from QSettings to UI.
 ***************************************************************/
void MainWindow::restoreCpuCudaSettings()
{
    QSettings s(kOrganizationName, kApplicationName);
    useCpuCheckBox->setChecked(s.value("settings/useCpu", false).toBool());
    cudaCheckBox->setChecked(s.value("settings/cuda", true).toBool());
}

/****************************************************************
 * @brief Detect OS System.
 ***************************************************************/
void MainWindow::detectSystem()
{
    // GPU detection (simple: check for NVIDIA)
    bool gpuDetected = false;
    QString os, release, version;
#if defined(Q_OS_WIN)
    os = "Windows";
    release = QSysInfo::productType();
    version = QSysInfo::productVersion();
#elif defined(Q_OS_MAC)
    os = "Mac";
    release = QSysInfo::productType();
    version = QSysInfo::productVersion();
#elif defined(Q_OS_LINUX)
    os = "Linux";
    QFile f("/etc/os-release");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!f.atEnd())
        {
            QString line = f.readLine();
            if (line.startsWith("ID="))
                release = line.mid(3).trimmed().replace("\"", "");
            if (line.startsWith("VERSION_ID="))
                version = line.mid(11).trimmed().replace("\"", "");
        }
    }
#endif
    osEdit->setText(os);
    osReleaseEdit->setText(release);
    osVersionEdit->setText(version);

#if defined(Q_OS_WIN)
    QProcess proc;
    proc.start("wmic path win32_VideoController get name");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    if (output.contains("NVIDIA", Qt::CaseInsensitive))
        gpuDetected = true;
#elif defined(Q_OS_LINUX)
    QProcess proc;
    proc.start("lspci");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    if (output.contains("NVIDIA", Qt::CaseInsensitive))
        gpuDetected = true;
#elif defined(Q_OS_MAC)
    QProcess proc;
    proc.start("system_profiler SPDisplaysDataType");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    if (output.contains("NVIDIA", Qt::CaseInsensitive))
        gpuDetected = true;
#endif
    if (!gpuDetected)
    {
        gpuDetected = detectNvidiaGpu();
    }
    gpuDetectedCheckBox->setChecked(gpuDetected);

    // Save to QSettings
    QSettings s(kOrganizationName, kApplicationName);
    s.setValue("settings/os", os);
    s.setValue("settings/osRelease", release);
    s.setValue("settings/osVersion", version);
    s.setValue("settings/gpuDetected", gpuDetected);
    if (gpuDetected)
    {
        queueStatusMessage(tr("GPU Detected"), 5000);
    }
    else
    {
        queueStatusMessage(tr("GPU Not Detected"), 5000);
    }
}

/****************************************************************
 * @brief on Create Venv.
 ***************************************************************/
void MainWindow::onCreateVenv()
{
    // Get Python version from settings
    QString pythonVersion = pythonVersionEdit->text().trimmed();
    if (pythonVersion.isEmpty())
    {
        pythonVersion = DEFAULT_PYTHON_VERSION;
    }

    // Get pip and pip-tools versions (currently informational)
    QString pipVersion = pipVersionEdit->text().trimmed();
    QString pipToolsVersion = pipToolsVersionEdit->text().trimmed();
    Q_UNUSED(pipVersion);
    Q_UNUSED(pipToolsVersion);

    // Set venv path
    QString venvPath = QDir::currentPath() + "/.venv";
    terminalEngine->setVenvPath(venvPath);

    // Switch to terminal tab
    mainTabs->setCurrentWidget(tabTerminal);

    // Clear terminal and show progress
    terminalOutput->clear();
    appendTerminalOutput("=== Creating Virtual Environment ===", false);
    appendTerminalOutput(QString("Python version: %1").arg(pythonVersion), false);
    appendTerminalOutput(QString("Virtual environment path: %1").arg(venvPath), false);
    appendTerminalOutput("", false);

    // UI state during creation
    QApplication::setOverrideCursor(Qt::WaitCursor);
    runCommandBtn->setEnabled(false);
    stopCommandBtn->setEnabled(false);

    bool success = terminalEngine->createVirtualEnvironment(pythonVersion);

    runCommandBtn->setEnabled(true);
    stopCommandBtn->setEnabled(false);
    QApplication::restoreOverrideCursor();

    int terminalTabIndex = mainTabs->indexOf(tabTerminal);

    if (success)
    {
        appendTerminalOutput("", false);
        appendTerminalOutput("=== Virtual Environment Ready ===", false);
        appendTerminalOutput("You can now run Python commands, pip, and pip-tools", false);

        // Try to activate and enable Terminal tab
        if (terminalEngine->activateVenv())
        {
            if (terminalTabIndex >= 0)
            {
                mainTabs->setTabEnabled(terminalTabIndex, true);
            }
            queueStatusMessage(tr("Virtual environment created and activated"), 5000);
        }
        else
        {
            queueStatusMessage(tr("Virtual environment created, but activation failed"), 5000);
        }
    }
    else
    {
        appendTerminalOutput("", false);
        appendTerminalOutput("=== Virtual Environment Creation Failed ===", true);
        if (terminalTabIndex >= 0)
        {
            mainTabs->setTabEnabled(terminalTabIndex, false);
        }
        queueStatusMessage(tr("Failed to create virtual environment"), 5000);
    }
}

/****************************************************************
 * @brief on Run Command.
 ***************************************************************/
void MainWindow::onRunCommand()
{
    QString command = commandInput->text().trimmed();
    if (command.isEmpty())
    {
        return;
    }

    // Clear input
    commandInput->clear();

    // Execute command
    terminalEngine->executeCommand(command);
}

/****************************************************************
 * @brief on Clear Terminal.
 ***************************************************************/
void MainWindow::onClearTerminal()
{
    terminalOutput->clear();
}

/****************************************************************
 * @brief on Stop Command.
 ***************************************************************/
void MainWindow::onStopCommand()
{
    terminalEngine->stopCurrentProcess();
}

/****************************************************************
 * @brief Handles terminal output from TerminalEngine.
 ***************************************************************/
void MainWindow::onTerminalOutput(const QString &output, bool isError)
{
    appendTerminalOutput(output, isError);
}

/****************************************************************
 * @brief Handles terminal command started event.
 ***************************************************************/
void MainWindow::onTerminalCommandStarted(const QString &command)
{
    runCommandBtn->setEnabled(false);
    stopCommandBtn->setEnabled(true);
    queueStatusMessage(QString("Executing: %1").arg(command), 5000);
}

/****************************************************************
 * @brief Handles terminal command finished event.
 ***************************************************************/
void MainWindow::onTerminalCommandFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    runCommandBtn->setEnabled(true);
    stopCommandBtn->setEnabled(false);

    if (exitStatus == QProcess::NormalExit && exitCode == 0)
    {
        queueStatusMessage(tr("Command completed successfully"), 5000);
    }
    else
    {
        queueStatusMessage(tr("Command failed"), 5000);
    }
}

/****************************************************************
 * @brief Handles venv progress messages.
 ***************************************************************/
void MainWindow::onVenvProgress(const QString &message)
{
    appendTerminalOutput(message, false);
    QApplication::processEvents(); // Update UI
}

/****************************************************************
 * @brief Helper to append text to terminal output with color.
 ***************************************************************/
void MainWindow::appendTerminalOutput(const QString &text, bool isError)
{
    if (text.isEmpty())
    {
        terminalOutput->appendPlainText("");
        return;
    }

    QTextCursor cursor = terminalOutput->textCursor();
    cursor.movePosition(QTextCursor::End);

    QTextCharFormat format;
    if (isError)
    {
        format.setForeground(QColor(Qt::red));
    }
    else if (text.startsWith("===") || text.startsWith("$"))
    {
        format.setForeground(QColor(Qt::blue));
        format.setFontWeight(QFont::Bold);
    }
    else
    {
        format.setForeground(QColor(Qt::black));
    }

    cursor.setCharFormat(format);
    cursor.insertText(text + "\n");

    terminalOutput->setTextCursor(cursor);
    terminalOutput->ensureCursorVisible();
}

/****************************************************************
 * @brief Sets up venv_running and venv_testing paths and saves to QSettings.
 ***************************************************************/
void MainWindow::setupVenvPaths()
{
    QSettings s(kOrganizationName, kApplicationName);
    QString projectRoot = QDir::currentPath();
    QString venvRunning = projectRoot + "/.venvs/venv_running";
    QString venvTesting = projectRoot + "/.venvs/venv_testing";
    s.setValue("venv/venv_running", venvRunning);
    s.setValue("venv/venv_testing", venvTesting);
}

/****************************************************************
 * @brief detect Gpu Via PowerShell.
 ***************************************************************/
bool MainWindow::detectGpuViaPowerShell()
{
    QProcess proc;
    proc.start(
        "powershell",
        QStringList() << "-Command"
                      << "Get-WmiObject Win32_VideoController | Select-Object -ExpandProperty Name");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    return output.contains("NVIDIA", Qt::CaseInsensitive);
}

/****************************************************************
 * @brief detect Gpu Via DxDiag.
 ***************************************************************/
bool MainWindow::detectGpuViaDxDiag()
{
    QProcess proc;
    proc.start("cmd", QStringList() << "/c" << "dxdiag /t dxdiag.txt");
    proc.waitForFinished();
    QFile file("dxdiag.txt");
    bool found = false;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        QString dxdiagOutput = file.readAll();
        found = dxdiagOutput.contains("NVIDIA", Qt::CaseInsensitive);
        file.remove(); // Clean up
    }
    return found;
}

/****************************************************************
 * @brief detect Gpu Via Nvidia Smi.
 ***************************************************************/
bool MainWindow::detectGpuViaNvidiaSmi()
{
    QProcess proc;
    proc.start("nvidia-smi");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    return output.contains("NVIDIA", Qt::CaseInsensitive);
}

/****************************************************************
 * @brief detect Nvidia Gpu.
 ***************************************************************/
bool MainWindow::detectNvidiaGpu()
{
    // Primary test: PowerShell WMI
    if (detectGpuViaPowerShell())
        return true;
    // Fallback 1: dxdiag
    if (detectGpuViaDxDiag())
        return true;
    // Fallback 2: nvidia-smi
    if (detectGpuViaNvidiaSmi())
        return true;
    // Not detected
    return false;
}

/****************************************************************
 * @brief on Search Package.
 ***************************************************************/
void MainWindow::onSearchPackage()
{
    QString pkg = packageNameInput->text().trimmed();
    if (pkg.isEmpty())
    {
        packageOutput->appendPlainText("Enter a package name to search.");
        return;
    }
    QProcess process;
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir::current().filePath("venv_running/Scripts/python.exe");
#else
    venvPython = QDir::current().filePath("venv_running/bin/python");
#endif
    QStringList args = {"-m", "pip", "search", pkg};
    process.start(venvPython, args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i)
    {
        installedPackagesList->addItem(lines.at(i));
    }
}

/****************************************************************
 * @brief Handles double-click on installed package list item.
 * @param index The model index of the double-clicked item.
 ***************************************************************/
void MainWindow::onInstalledPackagesListDoubleClicked(const QModelIndex &index)
{
    QString pkg = installedPackagesList->item(index.row())->text().split('=')[0];
    packageNameInput->setText(pkg);
    onUninstallPackage();
}

/****************************************************************
 * @brief on Install Package.
 ***************************************************************/
void MainWindow::onInstallPackage()
{
    QString pkg = packageNameInput->text().trimmed();
    if (pkg.isEmpty())
    {
        packageOutput->appendPlainText("Enter a package name to install.");
        return;
    }
    QProcess process;
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir::current().filePath("venv_running/Scripts/python.exe");
#else
    venvPython = QDir::current().filePath("venv_running/bin/python");
#endif
    QStringList args = {"-m", "pip", "install", pkg};
    process.start(venvPython, args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    if (!output.isEmpty())
        packageOutput->appendPlainText(output);
    if (!error.isEmpty())
        packageOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief on Uninstall Package.
 ***************************************************************/
void MainWindow::onUninstallPackage()
{
    QString pkg = packageNameInput->text().trimmed();
    if (pkg.isEmpty())
    {
        packageOutput->appendPlainText("Enter a package name to uninstall.");
        return;
    }
    QProcess process;
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir::current().filePath("venv_running/Scripts/python.exe");
#else
    venvPython = QDir::current().filePath("venv_running/bin/python");
#endif
    QStringList args = {"-m", "pip", "uninstall", "-y", pkg};
    process.start(venvPython, args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();
    if (!output.isEmpty())
        packageOutput->appendPlainText(output);
    if (!error.isEmpty())
        packageOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief Refreshes the list of installed packages in venv.
 ***************************************************************/
void MainWindow::refreshInstalledPackages()
{
    installedPackagesList->clear();
    QProcess process;
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir::current().filePath("venv_running/Scripts/python.exe");
#else
    venvPython = QDir::current().filePath("venv_running/bin/python");
#endif
    QStringList args = {"-m", "pip", "list", "--format=freeze"};
    process.start(venvPython, args);
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (int i = 0; i < lines.size(); ++i)
    {
        installedPackagesList->addItem(lines.at(i));
    }
}

/****************************************************************
 * @brief Handles immediate UI update when Python version changes.
 * @param newVersion The new Python version string (e.g., "3.11").
 ***************************************************************/
void MainWindow::onPythonVersionChanged(const QString &newVersion)
{
    // Update the Settings field immediately
    if (pythonVersionEdit)
        pythonVersionEdit->setText(newVersion);

    // Reload other settings for consistency
    loadAppSettings();

    DEBUG_MSG() << "[DEBUG] UI updated to Python version:" << newVersion;
}

/****************************************************************
 * @brief Refreshes the Python version displayed in the Settings UI.
 ***************************************************************/
void MainWindow::refreshPythonVersionUI()
{
    QSettings settings;
    QString currentVersion = settings.value("PythonVersion", "3.10").toString();

    if (pythonVersionEdit)
        pythonVersionEdit->setText(currentVersion);
}

void MainWindow::queueStatusMessage(const QString& msg, int timeoutMs)
{
    statusQueue.append(msg);
    if (!statusTimer.isActive())
    {
        // Start immediately if idle
        statusBar->showMessage(statusQueue.takeFirst(), timeoutMs);
        statusTimer.start(timeoutMs);
    }
}

void MainWindow::showNextStatusMessage()
{
    statusTimer.stop();
    if (!statusQueue.isEmpty())
    {
        QString msg = statusQueue.takeFirst();
        int timeoutMs = 5000; // default timeout per message
        statusBar->showMessage(msg, timeoutMs);
        statusTimer.start(timeoutMs);
    }
    else
    {
        statusBar->clearMessage();
    }
}

/************** End of MainWindow.cpp ***************************/
