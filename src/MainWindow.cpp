/****************************************************************
 * @file MainWindow.cpp
 * @brief Implements the main application window.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.6
 * @date    2025-11-01
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Implements the main window logic: settings, history, menus,
 * file and URL loaders, logging, and UI dialog wiring.
 ***************************************************************/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QHeaderView>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QUrl>
#include <utility> // Add this to your includes for std::as_const
#include <QSysInfo>
#include <QProcess>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QDir>

/****************************************************************
 * @brief Globals for MainWindow.
 ***************************************************************/
QString MainWindow::appVersion = "1.0";
const QString DEFAULT_PYTHON_VERSION = "3.11";
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
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    requirementsModel(new QStandardItemModel(this)),
    requirementsView(nullptr),
    logView(nullptr),
    progress(nullptr),
    recentLocalMenu(nullptr),
    recentWebMenu(nullptr),
    maxHistoryItems(10)
{
    ui->setupUi(this);

    // Manually add Recent Local and Recent Web submenus after actionFetchRequirements
    QList<QAction*> actions = ui->menuFile->actions();
    int insertIndex = -1;
    for (int i = 0; i < actions.size(); ++i)
    {
        if (actions[i] == ui->actionFetchRequirements)
        {
            insertIndex = i;
            break;
        }
    }
    recentLocalMenu = new QMenu(tr("Recent Local"), this);
    recentWebMenu = new QMenu(tr("Recent Web"), this);
    if (insertIndex != -1)
    {
        ui->menuFile->insertMenu(actions[insertIndex + 1], recentLocalMenu);
        ui->menuFile->insertMenu(actions[insertIndex + 1], recentWebMenu);
    }
    else
    {
        ui->menuFile->addMenu(recentLocalMenu);
        ui->menuFile->addMenu(recentWebMenu);
    }

    requirementsView = ui->requirementsView;
    logView = ui->logView;
    localHistoryTable = ui->localHistoryTable;
    webHistoryTable = ui->webHistoryTable;
    localHistoryModel = new QStandardItemModel(this);
    webHistoryModel = new QStandardItemModel(this);
    localHistoryTable->setModel(localHistoryModel);
    webHistoryTable->setModel(webHistoryModel);
    progress = ui->progressBar;

    gpuDetectedCheckBox = ui->gpuDetectedCheckBox;
    useCpuCheckBox = ui->useCpuCheckBox;
    cudaCheckBox = ui->cudaCheckBox;
    osEdit = ui->osEdit;
    osReleaseEdit = ui->osReleaseEdit;
    osVersionEdit = ui->osVersionEdit;

    requirementsView->setModel(requirementsModel);
    requirementsView->setAlternatingRowColors(true);
    requirementsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    requirementsView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    requirementsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    requirementsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    requirementsView->resizeColumnsToContents();
    requirementsView->resizeRowsToContents();
    requirementsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    requirementsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    requirementsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);

    loadAppSettings();
    loadHistory();

    connect(ui->actionOpenRequirements, &QAction::triggered, this, &MainWindow::openLocalRequirements);
    connect(ui->actionFetchRequirements, &QAction::triggered, this, &MainWindow::fetchRequirementsFromUrl);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exitApp);

    if (ui->buttonBoxPreferences)
    {
        connect(ui->buttonBoxPreferences, &QDialogButtonBox::accepted, this, &MainWindow::saveAppSettings);
        QPushButton *applyBtn = ui->buttonBoxPreferences->button(QDialogButtonBox::Apply);
        if (applyBtn)
        {
            connect(applyBtn, &QPushButton::clicked, this, &MainWindow::saveAppSettings);
        }
    }
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAboutBox);
    connect(ui->actionViewReadme, &QAction::triggered, this, &MainWindow::showReadmeDialog);
    ui->mainTabs->setCurrentIndex(0);
    ui->mainTabs->setCurrentWidget(ui->tabMain);

    // History tables and models
    localHistoryTable = ui->localHistoryTable;
    webHistoryTable = ui->webHistoryTable;
    localHistoryModel = new QStandardItemModel(this);
    webHistoryModel = new QStandardItemModel(this);
    localHistoryTable->setModel(localHistoryModel);
    webHistoryTable->setModel(webHistoryModel);

    connect(localHistoryTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateLocalHistoryButtons);
    connect(webHistoryTable->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::updateWebHistoryButtons);
    connect(ui->actionCreateVenv, &QAction::triggered, this, &MainWindow::onCreateVenv);

    // Terminal tab
    connect(ui->runCommandBtn, &QPushButton::clicked, this, &MainWindow::onRunCommand);
    connect(ui->clearTerminalBtn, &QPushButton::clicked, this, &MainWindow::onClearTerminal);

    // Package Manager tab
    connect(ui->searchPackageBtn, &QPushButton::clicked, this, &MainWindow::onSearchPackage);
    connect(ui->installPackageBtn, &QPushButton::clicked, this, &MainWindow::onInstallPackage);
    connect(ui->uninstallPackageBtn, &QPushButton::clicked, this, &MainWindow::onUninstallPackage);

    connect(ui->mainTabs, &QTabWidget::currentChanged, this, [this](int index){
        if (ui->mainTabs->widget(index)->objectName() == "tabPackageManager") {
            refreshInstalledPackages();
        }
    });
    connect(ui->installedPackagesList, &QListWidget::doubleClicked, this, &MainWindow::onInstalledPackagesListDoubleClicked);

/*
 * Auto connected
    // Connect local history buttons
    connect(ui->localAddButton,    &QPushButton::clicked, this, &MainWindow::on_localAddButton_clicked);
    connect(ui->localEditButton,   &QPushButton::clicked, this, &MainWindow::on_localEditButton_clicked);
    connect(ui->localDeleteButton, &QPushButton::clicked, this, &MainWindow::on_localDeleteButton_clicked);
    connect(ui->localUpButton,     &QPushButton::clicked, this, &MainWindow::on_localUpButton_clicked);
    connect(ui->localDownButton,   &QPushButton::clicked, this, &MainWindow::on_localDownButton_clicked);

    // Connect web history buttons
    connect(ui->webAddButton,    &QPushButton::clicked, this, &MainWindow::on_webAddButton_clicked);
    connect(ui->webEditButton,   &QPushButton::clicked, this, &MainWindow::on_webEditButton_clicked);
    connect(ui->webDeleteButton, &QPushButton::clicked, this, &MainWindow::on_webDeleteButton_clicked);
    connect(ui->webUpButton,     &QPushButton::clicked, this, &MainWindow::on_webUpButton_clicked);
    connect(ui->webDownButton,   &QPushButton::clicked, this, &MainWindow::on_webDownButton_clicked);
*/
    // Refresh tables on startup
    refreshRecentMenus();
    refreshHistoryTables();
    checkAndRestoreSettings();

    detectSystem();         // Detect OS, release, version, GPU
    restoreCpuCudaSettings(); // Restore CPU/Cuda settings from QSettings
    setupVenvPaths();       // Set up venv_running and venv_testing paths
}

/****************************************************************
 * @brief Destructor for MainWindow.
 ***************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
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
    QString path = QFileDialog::getOpenFileName(this, tr("Open requirements.txt"), QString(), tr("Text Files (*.txt)"));
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
    QString inputUrl = QInputDialog::getText(
        this,
        tr("Fetch requirements"),
        tr("Enter URL:"),
        QLineEdit::Normal,
        "",
        &ok
    );
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
        QMessageBox::warning(
            this,
            tr("Invalid requirements.txt"),
            tr("Validation failed:\n%1").arg(errors.join("\n"))
        );
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
        QMessageBox::warning(
            this,
            tr("Download failed"),
            tr("Failed to fetch requirements from URL:\n%1").arg(url)
        );
        historyRecentWeb.removeAll(url);
        refreshRecentMenus();
        saveHistory();
        return;
    }
    const QStringList lines = QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);
    QStringList errors;
    if (!validateRequirementsWithErrors(lines, errors))
    {
        QMessageBox::warning(
            this,
            tr("Invalid requirements.txt"),
            tr("Fetched content failed validation:\n%1").arg(errors.join("\n"))
        );
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
    appendLog(tr("Fetched %1 requirements from URL: %2")
        .arg(requirementsModel->rowCount())
        .arg(url));
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
        QList<QStandardItem*> row;
        row << new QStandardItem(path);
        localHistoryModel->appendRow(row);
    }
    localHistoryTable->resizeColumnsToContents();

    webHistoryModel->clear();
    webHistoryModel->setHorizontalHeaderLabels({tr("Recent Web URLs")});
    for (const QString &url : std::as_const(webList))
    {
        QList<QStandardItem*> row;
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
        connect(act, &QAction::triggered, this, [this, path]()
        {
            loadRequirementsFromFile(path);
        });
    }
    if (!historyRecentLocal.isEmpty())
    {
        recentLocalMenu->addSeparator();
        QAction *clearLocal = recentLocalMenu->addAction(tr("Clear Local History"));
        connect(clearLocal, &QAction::triggered, this, [this]()
        {
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
        connect(act, &QAction::triggered, this, [this, url]()
        {
            loadRequirementsFromUrl(url);
        });
    }
    if (!historyRecentWeb.isEmpty())
    {
        recentWebMenu->addSeparator();
        QAction *clearWeb = recentWebMenu->addAction(tr("Clear Web History"));
        connect(clearWeb, &QAction::triggered, this, [this]()
        {
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
 * @brief Loads application settings from QSettings to UI.
 ***************************************************************/
void MainWindow::loadAppSettings()
{
    QSettings settings;
    maxHistoryItems = settings.value("app/maxItems", 10).toInt();
    validateAppSettings();
    updateUiFromSettings();
}

/****************************************************************
 * @brief Saves application settings from UI to QSettings.
 ***************************************************************/
void MainWindow::saveAppSettings()
{
    applySettingsFromUi();
    validateAppSettings();
    QSettings settings;
    settings.setValue("app/maxItems", maxHistoryItems);
    statusBar()->showMessage(tr("Settings saved"), 2000);
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
    refreshRecentMenus();
    saveHistory();
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
    maxHistoryItems = ui->spinMaxItems->value();
}

/****************************************************************
 * @brief Updates UI widgets to reflect current settings.
 ***************************************************************/
void MainWindow::updateUiFromSettings()
{
    ui->spinMaxItems->setMinimum(-1);
    ui->spinMaxItems->setMaximum(std::numeric_limits<int>::max());
    ui->spinMaxItems->setToolTip(tr("-1 = unlimited, 0 not allowed, ≥1 valid"));
    ui->spinMaxItems->setValue(maxHistoryItems);
}

/****************************************************************
 * @brief Writes requirements into a one-column model.
 ***************************************************************/
void MainWindow::writeTableToModel(const QStringList &lines)
{
    if (!requirementsModel || !requirementsView) { return; }
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
    QSplitter *splitter = qobject_cast<QSplitter*>(requirementsView->parentWidget());
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
    statusBar()->showMessage(tr("Compiled successfully"));
}

/****************************************************************
 * @brief Shows About dialog using current appVersion.
 ***************************************************************/
void MainWindow::showAboutBox()
{
    QMessageBox::about(
        this,
        tr("About Pip Matrix Resolver"),
        tr("<b>Pip Matrix Resolver</b><br>"
           "Cross-platform Qt tool to resolve "
           "Python dependency matrices.<br>"
           "Version %1").arg(appVersion)
        );
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
    QString path = QInputDialog::getText(this, tr("Add Local File"), tr("File path:"), QLineEdit::Normal, "", &ok);
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
    if (!idx.isValid()) return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentLocal").toStringList();
    QString oldValue = list.at(idx.row());
    bool ok;
    QString newValue = QInputDialog::getText(this, tr("Edit Local File"), tr("File path:"), QLineEdit::Normal, oldValue, &ok);
    if (!ok) return; // User cancelled, do nothing
    if (newValue.isEmpty()) return; // Optionally, do nothing if empty
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
    if (!idx.isValid()) return;
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
    if (!idx.isValid() || idx.row() == 0) return;
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
    if (!idx.isValid() || idx.row() >= list.size() - 1) return;
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
    QString url = QInputDialog::getText(this, tr("Add Web URL"), tr("URL:"), QLineEdit::Normal, "", &ok);
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
    if (!idx.isValid()) return;
    QSettings s(kOrganizationName, kApplicationName);
    QStringList list = s.value("history/recentWeb").toStringList();
    QString oldValue = list.at(idx.row());
    bool ok;
    QString newValue = QInputDialog::getText(this, tr("Edit Web URL"), tr("URL:"), QLineEdit::Normal, oldValue, &ok);
    if (!ok) return; // User cancelled, do nothing
    if (newValue.isEmpty()) return; // Optionally, do nothing if empty
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
    if (!idx.isValid()) return;
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
    if (!idx.isValid() || idx.row() == 0) return;
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
    if (!idx.isValid() || idx.row() >= list.size() - 1) return;
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

    ui->localEditButton->setEnabled(hasSelection);
    ui->localDeleteButton->setEnabled(hasSelection);

    // Up: enabled if selected and not first row, and more than 1 item
    bool canUp = hasSelection && rowCount > 1;
    if (canUp)
    {
        QModelIndex idx = sel->currentIndex();
        canUp = idx.isValid() && idx.row() > 0;
    }
    ui->localUpButton->setEnabled(canUp);

    // Down: enabled if selected and not last row, and more than 1 item
    bool canDown = hasSelection && rowCount > 1;
    if (canDown)
    {
        QModelIndex idx = sel->currentIndex();
        canDown = idx.isValid() && idx.row() < rowCount - 1;
    }
    ui->localDownButton->setEnabled(canDown);
}

/****************************************************************
 * @brief Enables/disables web history buttons based on selection and table size.
 ***************************************************************/
void MainWindow::updateWebHistoryButtons()
{
    QItemSelectionModel *sel = webHistoryTable->selectionModel();
    int rowCount = webHistoryModel->rowCount();
    bool hasSelection = sel->hasSelection();

    ui->webEditButton->setEnabled(hasSelection);
    ui->webDeleteButton->setEnabled(hasSelection);

    // Up: enabled if selected and not first row, and more than 1 item
    bool canUp = hasSelection && rowCount > 1;
    if (canUp)
    {
        QModelIndex idx = sel->currentIndex();
        canUp = idx.isValid() && idx.row() > 0;
    }
    ui->webUpButton->setEnabled(canUp);

    // Down: enabled if selected and not last row, and more than 1 item
    bool canDown = hasSelection && rowCount > 1;
    if (canDown)
    {
        QModelIndex idx = sel->currentIndex();
        canDown = idx.isValid() && idx.row() < rowCount - 1;
    }
    ui->webDownButton->setEnabled(canDown);
}

/****************************************************************
 * @brief save Settings.
 ***************************************************************/
void MainWindow::saveSettings()
{
    QSettings s(kOrganizationName, kApplicationName);
    s.setValue("settings/pythonVersion", ui->pythonVersionEdit->text());
    s.setValue("settings/pipVersion", ui->pipVersionEdit->text());
    s.setValue("settings/pipToolsVersion", ui->pipToolsVersionEdit->text());
    s.setValue("settings/maxItems", ui->spinMaxItems->value());
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
    ui->pythonVersionEdit->setText(pythonVersion);

    // pip version
    QString pipVersion = s.value("settings/pipVersion", DEFAULT_PIP_VERSION).toString();
    if (pipVersion.isEmpty())
    {
        pipVersion = DEFAULT_PIP_VERSION;
        s.setValue("settings/pipVersion", pipVersion);
    }
    ui->pipVersionEdit->setText(pipVersion);

    // pip-tools version
    QString pipToolsVersion = s.value("settings/pipToolsVersion", DEFAULT_PIPTOOLS_VERSION).toString();
    if (pipToolsVersion.isEmpty())
    {
        pipToolsVersion = DEFAULT_PIPTOOLS_VERSION;
        s.setValue("settings/pipToolsVersion", pipToolsVersion);
    }
    ui->pipToolsVersionEdit->setText(pipToolsVersion);

    // Max items
    int maxItems = s.value("settings/maxItems", DEFAULT_MAX_ITEMS).toInt();
    if (maxItems == 0)
    {
        maxItems = DEFAULT_MAX_ITEMS;
        s.setValue("settings/maxItems", maxItems);
    }
    ui->spinMaxItems->setValue(maxItems);
}

/****************************************************************
 * @brief Restores CPU and Cuda settings from QSettings to UI.
 ***************************************************************/
void MainWindow::restoreCpuCudaSettings()
{
    QSettings s(kOrganizationName, kApplicationName);
    ui->useCpuCheckBox->setChecked(s.value("settings/useCpu", false).toBool());
    ui->cudaCheckBox->setChecked(s.value("settings/cuda", true).toBool());
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
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            QString line = f.readLine();
            if (line.startsWith("ID=")) release = line.mid(3).trimmed().replace("\"", "");
            if (line.startsWith("VERSION_ID=")) version = line.mid(11).trimmed().replace("\"", "");
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
    if (output.contains("NVIDIA", Qt::CaseInsensitive)) gpuDetected = true;
#elif defined(Q_OS_LINUX)
    QProcess proc;
    proc.start("lspci");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    if (output.contains("NVIDIA", Qt::CaseInsensitive)) gpuDetected = true;
#elif defined(Q_OS_MAC)
    QProcess proc;
    proc.start("system_profiler SPDisplaysDataType");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    if (output.contains("NVIDIA", Qt::CaseInsensitive)) gpuDetected = true;
#endif
    if (!gpuDetected)
    {
        gpuDetected = detectNvidiaGpu();
        ui->gpuDetectedCheckBox->setChecked(gpuDetected);
    }
    // Save to QSettings
    QSettings s(kOrganizationName, kApplicationName);
    s.setValue("settings/os", os);
    s.setValue("settings/osRelease", release);
    s.setValue("settings/osVersion", version);
    s.setValue("settings/gpuDetected", gpuDetected);
    if (gpuDetected)
    {
        ui->statusbar->showMessage(tr("GPU Detected"));
    }
    else
    {
        ui->statusbar->showMessage(tr("GPU Not Detected"));
    }
}

/****************************************************************
 * @brief on Create Venv.
 ***************************************************************/
void MainWindow::onCreateVenv()
{
    // Check if python, pip, or pip-tools settings have changed
    // If so, rebuild venv_running
    // (You can compare current settings to those saved in QSettings)

    // Call your bash script using QProcess
    QProcess proc;
    QString script = "/path/to/pip-matrix-common.sh";
    QStringList args;
    args << "--create-venv-running"; // You can define this flag in your script
    proc.start(script, args);
    proc.waitForFinished(-1);
    QString output = proc.readAllStandardOutput();
    QMessageBox::information(this, tr("Create venv_running"), output);
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
    proc.start("powershell", QStringList() << "-Command" << "Get-WmiObject Win32_VideoController | Select-Object -ExpandProperty Name");
    proc.waitForFinished();
    QString output = proc.readAllStandardOutput();
    // Debug: print output if needed
    // QMessageBox::information(nullptr, "PowerShell GPU Output", output);
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
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString dxdiagOutput = file.readAll();
        found = dxdiagOutput.contains("NVIDIA", Qt::CaseInsensitive);
        file.remove(); // Clean up
    }
    // Debug: print output if needed
    // QMessageBox::information(nullptr, "dxdiag GPU Output", dxdiagOutput);
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
    // Debug: print output if needed
    // QMessageBox::information(nullptr, "nvidia-smi Output", output);
    return output.contains("NVIDIA", Qt::CaseInsensitive);
}

/****************************************************************
 * @brief detect Nvidia Gpu.
 ***************************************************************/
bool MainWindow::detectNvidiaGpu()
{
    // Primary test: PowerShell WMI
    if (detectGpuViaPowerShell()) return true;
    // Fallback 1: dxdiag
    if (detectGpuViaDxDiag()) return true;
    // Fallback 2: nvidia-smi
    if (detectGpuViaNvidiaSmi()) return true;
    // Not detected
    return false;
}

/****************************************************************
 * @brief on Run Command.
 ***************************************************************/
void MainWindow::onRunCommand()
{
    QString command = ui->commandInput->text().trimmed();
    if (command.isEmpty()) {
        ui->terminalOutput->appendPlainText("No command entered.");
        return;
    }

    QProcess process;
    QString venvPython;
#if defined(Q_OS_WIN)
    venvPython = QDir::current().filePath("venv_running/Scripts/python.exe");
#else
    venvPython = QDir::current().filePath("venv_running/bin/python");
#endif

    QStringList args;
    bool usedVenv = false;

    if (command.startsWith("pip ")) {
        args << "-m" << "pip";
        args << command.mid(4).split(' ');
        process.start(venvPython, args);
        usedVenv = true;
    } else if (command.startsWith("pip-compile")) {
        args << "-m" << "piptools" << "compile";
        QString rest = command.mid(QString("pip-compile").length()).trimmed();
        if (!rest.isEmpty()) args << rest.split(' ');
        process.start(venvPython, args);
        usedVenv = true;
    } else if (command.startsWith("pip-sync")) {
        args << "-m" << "piptools" << "sync";
        QString rest = command.mid(QString("pip-sync").length()).trimmed();
        if (!rest.isEmpty()) args << rest.split(' ');
        process.start(venvPython, args);
        usedVenv = true;
    } else if (command.startsWith("python ")) {
        args = command.mid(7).split(' ');
        process.start(venvPython, args);
        usedVenv = true;
    } else {
#if defined(Q_OS_WIN)
        process.start("cmd.exe", QStringList() << "/C" << command);
#else
        process.start("bash", QStringList() << "-c" << command);
#endif
    }

    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QString error = process.readAllStandardError();

    if (usedVenv)
        ui->terminalOutput->appendPlainText("[venv] " + command);

    if (!output.isEmpty())
        ui->terminalOutput->appendPlainText(output);
    if (!error.isEmpty())
        ui->terminalOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief on Clear Terminal.
 ***************************************************************/
void MainWindow::onClearTerminal()
{
    ui->terminalOutput->clear();
}

/****************************************************************
 * @brief on Search Package.
 ***************************************************************/
void MainWindow::onSearchPackage()
{
    QString pkg = ui->packageNameInput->text().trimmed();
    if (pkg.isEmpty()) {
        ui->packageOutput->appendPlainText("Enter a package name to search.");
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
    QString error = process.readAllStandardError();
    if (!output.isEmpty())
        ui->packageOutput->appendPlainText(output);
    if (!error.isEmpty())
        ui->packageOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief on Install Package.
 ***************************************************************/
void MainWindow::onInstallPackage()
{
    QString pkg = ui->packageNameInput->text().trimmed();
    if (pkg.isEmpty()) {
        ui->packageOutput->appendPlainText("Enter a package name to install.");
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
        ui->packageOutput->appendPlainText(output);
    if (!error.isEmpty())
        ui->packageOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief on Uninstall Package.
 ***************************************************************/
void MainWindow::onUninstallPackage()
{
    QString pkg = ui->packageNameInput->text().trimmed();
    if (pkg.isEmpty()) {
        ui->packageOutput->appendPlainText("Enter a package name to uninstall.");
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
        ui->packageOutput->appendPlainText(output);
    if (!error.isEmpty())
        ui->packageOutput->appendPlainText("[ERROR] " + error);
}

/****************************************************************
 * @brief .
 ***************************************************************/
void MainWindow::refreshInstalledPackages()
{
    ui->installedPackagesList->clear();
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
        ui->installedPackagesList->addItem(lines.at(i));
    }
}

/****************************************************************
 * @brief .
 ***************************************************************/
void MainWindow::onInstalledPackagesListDoubleClicked(const QModelIndex &index)
{
    QString pkg = ui->installedPackagesList->item(index.row())->text().split('=')[0];
    ui->packageNameInput->setText(pkg);
    onUninstallPackage();
}

/****************************************************************
 * @brief .
 ***************************************************************/

/****************************************************************
 * @brief .
 ***************************************************************/

/****************************************************************
 * @brief .
 ***************************************************************/

/****************************************************************
 * @brief .
 ***************************************************************/

/************** End of MainWindow.cpp **************************/
