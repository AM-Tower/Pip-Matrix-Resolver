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
 * file and URL loaders, logging, and UI dialog wiring. All
 * design stays in .ui; this file wires UI to behavior.
 ***************************************************************/

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QStandardItemModel>
#include <QTableView>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QMenu>
#include <QAction>
#include <QDateTime>
#include <QDialog>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QDialogButtonBox>
#include <limits>
#include <QStringList>
#include <QByteArray>
#include <QDebug>
#include <QTimer>

namespace MatrixUtility
{
QStringList readTextFileLines(const QString &path);
bool validateRequirementsWithErrors(const QStringList &lines, QStringList &errors);
void writeTableToModel(QStandardItemModel *model, const QStringList &lines);
QString normalizeRawUrl(const QString &inputUrl);
bool downloadText(const QString &url, QByteArray &out);
QString logsDir();
}

/****************************************************************
 * @brief Constructor.
 ***************************************************************/
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    resolver(nullptr),
    venv(nullptr),
    batch(nullptr),
    requirementsModel(new QStandardItemModel(this)),
    requirementsView(nullptr),
    logView(nullptr),
    progress(nullptr),
    recentLocalMenu(nullptr),
    recentWebMenu(nullptr),
    historyWidget(nullptr),
    maxHistoryItems(10),
    appVersion(QStringLiteral("1.0"))
{
    ui->setupUi(this);

    // Bind core widgets created in Designer
    requirementsView = ui->requirementsView;
    logView = ui->logView;
    progress = ui->progressBar;
    historyWidget = ui->matrixHistoryWidget;

    // Bind menus directly from Designer
    ensureMenusInitialized();

    // Attach the model once, right after setupUi
    requirementsView->setModel(requirementsModel);
    requirementsView->setAlternatingRowColors(true);
    requirementsView->setSelectionBehavior(QAbstractItemView::SelectRows);
    requirementsView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Settings and history
    loadAppSettings();
    loadHistory();

    // Defer menu population until UI is fully realized
    QTimer::singleShot(0, this, &MainWindow::onInitialized);

    // Connect actions
    connect(ui->actionOpenRequirements, &QAction::triggered,
            this, &MainWindow::openLocalRequirements);
    connect(ui->actionFetchRequirements, &QAction::triggered,
            this, &MainWindow::fetchRequirementsFromUrl);

    if (ui->buttonBoxPreferences)
    {
        connect(ui->buttonBoxPreferences, &QDialogButtonBox::accepted,
                this, &MainWindow::saveAppSettings);
        QPushButton *applyBtn =
            ui->buttonBoxPreferences->button(QDialogButtonBox::Apply);
        if (applyBtn)
        {
            connect(applyBtn, &QPushButton::clicked,
                    this, &MainWindow::saveAppSettings);
        }
    }
}

/****************************************************************
 * @brief Destructor.
 ***************************************************************/
MainWindow::~MainWindow()
{
    delete ui;
}

/****************************************************************
 * @brief Called once after construction to finalize setup.
 ***************************************************************/
void MainWindow::onInitialized()
{
    refreshRecentMenus();
    ensureViewScrollable();
}

/****************************************************************
 * @brief Ensure menus exist and bind from Designer.
 ***************************************************************/
void MainWindow::ensureMenusInitialized()
{
    recentLocalMenu = ui->menuRecentLocal;
    recentWebMenu   = ui->menuRecentWeb;

    if (!recentLocalMenu || !recentWebMenu)
    {
        qWarning() << "Menus missing in UI. Check .ui file.";
    }
}

/****************************************************************
 * @brief Ensure requirementsView is scrollable and sized.
 ***************************************************************/
void MainWindow::ensureViewScrollable()
{
    if (!requirementsView)
    {
        requirementsView = ui ? ui->requirementsView : nullptr;
        if (!requirementsView)
        {
            qWarning() << "ensureViewScrollable: requirementsView still null";
            return;
        }
    }

    if (!requirementsView->model())
    {
        qWarning() << "ensureViewScrollable: no model set, attaching default";
        requirementsView->setModel(requirementsModel);
    }

    requirementsView->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    requirementsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    requirementsView->resizeColumnsToContents();
    requirementsView->resizeRowsToContents();

    requirementsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
    requirementsView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    requirementsView->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
}

/****************************************************************
 * @brief Open requirements from a local file.
 ***************************************************************/
void MainWindow::openLocalRequirements()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open requirements.txt"),
        QString(),
        tr("Text Files (*.txt)")
        );

    if (path.isEmpty())
    {
        return;
    }

    loadRequirementsFromFile(path);
}

/****************************************************************
 * @brief Fetch requirements from a URL via dialog.
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

    const QString rawUrl = MatrixUtility::normalizeRawUrl(inputUrl);
    loadRequirementsFromUrl(rawUrl);
}

/****************************************************************
 * @brief Load requirements from a local file.
 ***************************************************************/
void MainWindow::loadRequirementsFromFile(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }

    if (!QFile::exists(path))
    {
        QMessageBox::warning(
            this,
            tr("File missing"),
            tr("File no longer exists:\n%1").arg(path)
            );

        historyRecentLocal.removeAll(path);
        refreshRecentMenus();
        saveHistory();
        return;
    }

    const QStringList lines = MatrixUtility::readTextFileLines(path);

    QStringList errors;
    if (!MatrixUtility::validateRequirementsWithErrors(lines, errors))
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
        MatrixUtility::writeTableToModel(requirementsModel, lines);
    }

    applyToolsEnabled(true);

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

    appendLog(tr("Loaded %1 requirements from %2")
                  .arg(requirementsModel->rowCount())
                  .arg(path));
}

/****************************************************************
 * @brief Load requirements from a URL.
 ***************************************************************/
void MainWindow::loadRequirementsFromUrl(const QString &url)
{
    if (url.isEmpty())
    {
        return;
    }

    QByteArray content;
    if (!MatrixUtility::downloadText(url, content))
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

    const QStringList lines =
        QString::fromUtf8(content).split('\n', Qt::KeepEmptyParts);

    QStringList errors;
    if (!MatrixUtility::validateRequirementsWithErrors(lines, errors))
    {
        QMessageBox::warning(
            this,
            tr("Invalid requirements.txt"),
            tr("Fetched content failed validation:\n%1")
                .arg(errors.join("\n"))
            );
        return;
    }

    if (requirementsModel)
    {
        requirementsModel->clear();
        MatrixUtility::writeTableToModel(requirementsModel, lines);
    }

    ensureViewScrollable();
    applyToolsEnabled(true);

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
 * @brief Refresh recent file and URL menus safely.
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
    for (const QString &path : historyRecentLocal)
    {
        QAction *act = recentLocalMenu->addAction(path);
        connect(act, &QAction::triggered, this, [this, path]() {
            loadRequirementsFromFile(path);
        });
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
    for (const QString &url : historyRecentWeb)
    {
        QAction *act = recentWebMenu->addAction(url);
        connect(act, &QAction::triggered, this, [this, url]() {
            loadRequirementsFromUrl(url);
        });
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
 * @brief Clear all history (local + web).
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
 * @brief Save persistent history to QSettings.
 ***************************************************************/
void MainWindow::saveHistory()
{
    QSettings s;
    s.setValue("history/recentLocal", historyRecentLocal);
    s.setValue("history/recentWeb", historyRecentWeb);
}

/****************************************************************
 * @brief Load persistent history from QSettings.
 ***************************************************************/
void MainWindow::loadHistory()
{
    QSettings s;
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
 * @brief Load application settings and reflect in UI.
 ***************************************************************/
void MainWindow::loadAppSettings()
{
    QSettings settings;
    maxHistoryItems = settings.value("app/maxItems", 10).toInt();
    appVersion = settings.value("app/version", "1.0").toString();

    validateAppSettings();
    updateUiFromSettings();
}

/****************************************************************
 * @brief Save application settings from UI to QSettings.
 ***************************************************************/
void MainWindow::saveAppSettings()
{
    applySettingsFromUi();
    validateAppSettings();

    QSettings settings;
    settings.setValue("app/maxItems", maxHistoryItems);
    settings.setValue("app/version", appVersion);

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
 * @brief Validate and clamp all settings to safe ranges.
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

    QStringList parts = appVersion.split('.');
    bool valid = true;
    for (const QString &seg : parts)
    {
        if (seg.trimmed() == QStringLiteral("0"))
        {
            valid = false;
            break;
        }
    }
    if (!valid || appVersion.trimmed().isEmpty())
    {
        appVersion = QStringLiteral("1.0");
    }
}

/****************************************************************
 * @brief Apply settings from UI widgets to internal state.
 ***************************************************************/
void MainWindow::applySettingsFromUi()
{
    maxHistoryItems = ui->spinMaxItems->value();
    appVersion = ui->editVersion->text();
}

/****************************************************************
 * @brief Update UI widgets to reflect current settings.
 ***************************************************************/
void MainWindow::updateUiFromSettings()
{
    ui->spinMaxItems->setMinimum(-1);
    ui->spinMaxItems->setMaximum(std::numeric_limits<int>::max());
    ui->spinMaxItems->setToolTip(tr("-1 = unlimited, 0 not allowed, ≥1 valid"));
    ui->spinMaxItems->setValue(maxHistoryItems);

    ui->editVersion->setToolTip(tr("Version string, no segment may be \"0\""));
    ui->editVersion->setText(appVersion);
}

/****************************************************************
 * @brief Start matrix resolution.
 ***************************************************************/
void MainWindow::startResolve()
{
    appendLog(tr("Starting matrix resolution..."));
    if (resolver)
    {
        resolver->start();
    }
}

/****************************************************************
 * @brief Pause matrix resolution.
 ***************************************************************/
void MainWindow::pauseResolve()
{
    appendLog(tr("Pausing..."));
    if (resolver)
    {
        resolver->pause();
    }
}

/****************************************************************
 * @brief Resume matrix resolution.
 ***************************************************************/
void MainWindow::resumeResolve()
{
    appendLog(tr("Resuming..."));
    if (resolver)
    {
        resolver->resume();
    }
}

/****************************************************************
 * @brief Stop matrix resolution.
 ***************************************************************/
void MainWindow::stopResolve()
{
    appendLog(tr("Stopping..."));
    if (resolver)
    {
        resolver->stop();
    }
}

/****************************************************************
 * @brief Append a line to the log view.
 ***************************************************************/
void MainWindow::appendLog(const QString &line)
{
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    logView->appendPlainText(QString("[%1] %2").arg(time, line));
}

/****************************************************************
 * @brief Update progress bar.
 ***************************************************************/
void MainWindow::updateProgress(int percent)
{
    progress->setValue(percent);
}

/****************************************************************
 * @brief Show compiled result message.
 ***************************************************************/
void MainWindow::showCompiledResult(const QString &path)
{
    appendLog(tr("Successfully compiled: %1").arg(path));
    statusBar()->showMessage(tr("Compiled successfully"));
}

/****************************************************************
 * @brief Show About dialog.
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
 * @brief Show README dialog.
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

    connect(&closeButton, &QPushButton::clicked,
            &dialog, &QDialog::accept);

    dialog.exec();
}

/****************************************************************
 * @brief Open matrix history tab.
 ***************************************************************/
void MainWindow::openMatrixHistory()
{
    ui->mainTabs->setCurrentWidget(ui->tabHistory);
}

void MainWindow::applyToolsEnabled(bool enabled)
{
    ui->actionCreateVenv->setEnabled(enabled);
    ui->actionResolveMatrix->setEnabled(enabled);
    ui->actionPause->setEnabled(enabled);
    ui->actionResume->setEnabled(enabled);
    ui->actionStop->setEnabled(enabled);
    ui->actionRunBatch->setEnabled(enabled);
}

/************** End of MainWindow.cpp **************************/
