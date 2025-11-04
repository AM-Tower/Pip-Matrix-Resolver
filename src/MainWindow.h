/****************************************************************
 * @file MainWindow.h
 * @brief Declares the main application window class.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.7
 * @date    2025-11-03
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Main window interface for PipMatrixResolver Qt application.
 * Includes settings API, history, menus, and shared loaders for
 * files and URLs. Uses C-style braces. Now with dynamic UI.
 ***************************************************************/

#pragma once
#include <QMainWindow>
#include <QStringList>
#include <QTimer>
#include <QStandardItemModel>
#include <QTableView>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QByteArray>
#include <QDialog>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QDebug>
#include <QTabWidget>
#include <QSplitter>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QListWidget>
#include "CommandsTab.h"

/****************************************************************
 * @class MainWindow
 * @brief Implements the main application window.
 ***************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // QSettings
    static const QString kOrganizationName;
    static const QString kApplicationName;
    // Globals
    static QString appVersion;

private slots:
    void openLocalRequirements();
    void fetchRequirementsFromUrl();
    void refreshRecentMenus();
    void clearAllHistory();
    void loadAppSettings();
    void saveAppSettings();
    void validateAppSettings();
    void applySettingsFromUi();
    void updateUiFromSettings();
    void startResolve();
    void pauseResolve();
    void resumeResolve();
    void stopResolve();
    void appendLog(const QString &line);
    void updateProgress(int percent);
    void showCompiledResult(const QString &path);
    void showAboutBox();
    void showReadmeDialog();
    void exitApp();
    void writeTableToModel(const QStringList &lines);
    // Local history slots
    void on_localAddButton_clicked();
    void on_localEditButton_clicked();
    void on_localDeleteButton_clicked();
    void on_localUpButton_clicked();
    void on_localDownButton_clicked();

    // Web history slots
    void on_webAddButton_clicked();
    void on_webEditButton_clicked();
    void on_webDeleteButton_clicked();
    void on_webUpButton_clicked();
    void on_webDownButton_clicked();

    void updateLocalHistoryButtons();
    void updateWebHistoryButtons();
    // Slot for menu action
    void onCreateVenv();

    void onRunCommand();
    void onClearTerminal();

    // Package Manager tab
    void onSearchPackage();
    void onInstallPackage();
    void onUninstallPackage();

    void refreshInstalledPackages();
    void onInstalledPackagesListDoubleClicked(const QModelIndex &index);

private:
    void setupUi();
    void loadRequirementsFromFile(const QString &path);
    void loadRequirementsFromUrl(const QString &url);
    void saveHistory();
    void loadHistory();
    /****************************************************************
     * @brief Populates the local and web history tables in the
     *        History tab with the current history data.
     ***************************************************************/
    void refreshHistoryTables();
    /****************************************************************
    * @brief Checks all settings in the Settings tab at startup,
    *        restores defaults if missing, and updates the UI.
    ***************************************************************/
    void checkAndRestoreSettings();

    bool detectGpuViaPowerShell();
    bool detectNvidiaGpu();
    bool detectGpuViaNvidiaSmi();
    bool detectGpuViaDxDiag();
    /****************************************************************
    * @brief Saves all settings from the Settings tab to QSettings.
    ***************************************************************/
    void saveSettings();
    // Startup/system functions
    void detectSystem();
    void restoreCpuCudaSettings();
    void setupVenvPaths();

    // Utility functions moved from MatrixUtility
    QStringList readTextFileLines(const QString &path);
    bool validateRequirementsWithErrors(const QStringList &lines, QStringList &errors);
    QString normalizeRawUrl(const QString &inputUrl);
    bool downloadText(const QString &url, QByteArray &out);
    QString logsDir();

    // History data
    QStringList historyRecentLocal;
    QStringList historyRecentWeb;

    // UI widgets - Main Window
    QWidget *centralWidget;
    QTabWidget *mainTabs;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    // Menus
    QMenu *menuFile;
    QMenu *menuTools;
    QMenu *menuBatch;
    QMenu *menuHelp;

    // Tab: Commands
    QWidget *tabCommands;
    CommandsTab *commandsTab;

    // Actions
    QAction *actionOpenRequirements;
    QAction *actionFetchRequirements;
    QAction *actionExit;
    QAction *actionCreateVenv;
    QAction *actionResolveMatrix;
    QAction *actionPause;
    QAction *actionResume;
    QAction *actionStop;
    QAction *actionRunBatch;
    QAction *actionAbout;
    QAction *actionViewReadme;

    // Tab: Main
    QWidget *tabMain;
    QSplitter *splitter;
    QSplitter *bottomSplitter;
    QStandardItemModel *requirementsModel;
    QTableView *requirementsView;
    QTableView *matrixView;
    QPlainTextEdit *logView;
    QProgressBar *progress;

    // Tab: History
    QWidget *tabHistory;
    QTableView *localHistoryTable;
    QTableView *webHistoryTable;
    QStandardItemModel *localHistoryModel;
    QStandardItemModel *webHistoryModel;
    QPushButton *localAddButton;
    QPushButton *localEditButton;
    QPushButton *localDeleteButton;
    QPushButton *localUpButton;
    QPushButton *localDownButton;
    QPushButton *webAddButton;
    QPushButton *webEditButton;
    QPushButton *webDeleteButton;
    QPushButton *webUpButton;
    QPushButton *webDownButton;

    // Tab: Terminal
    QWidget *tabTerminal;
    QPlainTextEdit *terminalOutput;
    QLineEdit *commandInput;
    QPushButton *runCommandBtn;
    QPushButton *clearTerminalBtn;

    // Tab: Package Manager
    QWidget *tabPackageManager;
    QLineEdit *packageNameInput;
    QPushButton *searchPackageBtn;
    QPushButton *installPackageBtn;
    QPushButton *uninstallPackageBtn;
    QListWidget *installedPackagesList;
    QPlainTextEdit *packageOutput;

    // Tab: Settings
    QWidget *tabSettings;
    QLineEdit *pythonVersionEdit;
    QLineEdit *pipVersionEdit;
    QLineEdit *pipToolsVersionEdit;
    QSpinBox *spinMaxItems;
    QCheckBox *gpuDetectedCheckBox;
    QCheckBox *useCpuCheckBox;
    QCheckBox *cudaCheckBox;
    QLineEdit *osEdit;
    QLineEdit *osReleaseEdit;
    QLineEdit *osVersionEdit;
    QPushButton *saveSettingsButton;
    QPushButton *restoreDefaultsButton;
    QDialogButtonBox *buttonBoxPreferences;

    QMenu *recentLocalMenu;
    QMenu *recentWebMenu;

    // Venv paths
    QString venvRunningPath;
    QString venvTestingPath;

    // Settings
    int maxHistoryItems; // -1=unlimited, 0 invalid, ≥1 valid
};

/************** End of MainWindow.h ****************************/
