/****************************************************************
 * @file MainWindow.h
 * @brief Declares the main application window class.
 *
 * @author Jeffrey Scott Flesher
 * @version 0.6
 * @date    2025-11-01
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Main window interface for PipMatrixResolver Qt application.
 * Includes forward declarations, settings API, history, menus,
 * and shared loaders for files and URLs. Uses C-style braces.
 ***************************************************************/

#pragma once

#include <QMainWindow>
#include <QStringList>
#include "ResolverEngine.h"
#include "VenvManager.h"
#include "BatchRunner.h"
#include "MatrixHistory.h"

/****************************************************************
 * @class ResolverEngine
 * @brief Forward declaration for resolver engine.
 ***************************************************************/
class ResolverEngine;

/****************************************************************
 * @class VenvManager
 * @brief Forward declaration for venv manager.
 ***************************************************************/
class VenvManager;

/****************************************************************
 * @class BatchRunner
 * @brief Forward declaration for batch runner.
 ***************************************************************/
class BatchRunner;

/****************************************************************
 * @class MatrixHistory
 * @brief Forward declaration for history widget.
 ***************************************************************/
class MatrixHistory;

/****************************************************************
 * @class QStandardItemModel
 * @brief Forward declaration for model.
 ***************************************************************/
class QStandardItemModel;

/****************************************************************
 * @class QPlainTextEdit
 * @brief Forward declaration for log view.
 ***************************************************************/
class QPlainTextEdit;

/****************************************************************
 * @class QProgressBar
 * @brief Forward declaration for progress bar.
 ***************************************************************/
class QProgressBar;

/****************************************************************
 * @class QTableView
 * @brief Forward declaration for table view.
 ***************************************************************/
class QTableView;

/****************************************************************
 * @class QMenu
 * @brief Forward declaration for menu.
 ***************************************************************/
class QMenu;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/****************************************************************
 * @class MainWindow
 * @brief Central application window for PipMatrixResolver.
 ***************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /************************************************************
     * @brief Construct the main window.
     * @param parent Parent widget pointer.
     ***********************************************************/
    explicit MainWindow(QWidget *parent = nullptr);

    /************************************************************
     * @brief Destroy the main window.
     ***********************************************************/
    ~MainWindow();

private slots:
    /************************************************************
     * @brief Open requirements from a local file via dialog.
     ***********************************************************/
    void openLocalRequirements();

    /************************************************************
     * @brief Fetch requirements from a URL via dialog.
     ***********************************************************/
    void fetchRequirementsFromUrl();

    /************************************************************
     * @brief Refresh recent menus for local and web items.
     ***********************************************************/
    void refreshRecentMenus();

    /************************************************************
     * @brief Clear both local and web recent history lists.
     ***********************************************************/
    void clearAllHistory();

    /************************************************************
     * @brief Load application settings from QSettings to UI.
     ***********************************************************/
    void loadAppSettings();

    /************************************************************
     * @brief Save application settings from UI to QSettings.
     ***********************************************************/
    void saveAppSettings();

    /************************************************************
     * @brief Validate and clamp settings to safe ranges.
     ***********************************************************/
    void validateAppSettings();

    /************************************************************
     * @brief Apply settings from UI widgets to member state.
     ***********************************************************/
    void applySettingsFromUi();

    /************************************************************
     * @brief Update UI widgets to reflect current settings.
     ***********************************************************/
    void updateUiFromSettings();

    /************************************************************
     * @brief Start matrix resolution (existing behavior).
     ***********************************************************/
    void startResolve();

    /************************************************************
     * @brief Pause matrix resolution (existing behavior).
     ***********************************************************/
    void pauseResolve();

    /************************************************************
     * @brief Resume matrix resolution (existing behavior).
     ***********************************************************/
    void resumeResolve();

    /************************************************************
     * @brief Stop matrix resolution (existing behavior).
     ***********************************************************/
    void stopResolve();

    /************************************************************
     * @brief Append a line to the log view with timestamp.
     * @param line The message line to append.
     ***********************************************************/
    void appendLog(const QString &line);

    /************************************************************
     * @brief Update progress bar percent.
     * @param percent The progress value (0-100).
     ***********************************************************/
    void updateProgress(int percent);

    /************************************************************
     * @brief Show compiled result message path.
     * @param path The compiled output path.
     ***********************************************************/
    void showCompiledResult(const QString &path);

    /************************************************************
     * @brief Show About dialog using current appVersion.
     ***********************************************************/
    void showAboutBox();

    /************************************************************
     * @brief Show README dialog from resources.
     ***********************************************************/
    void showReadmeDialog();

    /************************************************************
     * @brief Navigate to MatrixHistory tab in main UI.
     ***********************************************************/
    void openMatrixHistory();

private:
    /************************************************************
     * @brief Load requirements directly from file path.
     * @param path Absolute or canonical file path.
     ***********************************************************/
    void loadRequirementsFromFile(const QString &path);

    /************************************************************
     * @brief Load requirements directly from URL.
     * @param url Normalized URL string.
     ***********************************************************/
    void loadRequirementsFromUrl(const QString &url);

    /************************************************************
     * @brief Save recent history lists to QSettings.
     ***********************************************************/
    void saveHistory();

    /************************************************************
     * @brief Load recent history lists from QSettings.
     ***********************************************************/
    void loadHistory();

    void applyToolsEnabled(bool enabled);

private:
    Ui::MainWindow *ui;

    // Engines
    ResolverEngine *resolver;
    VenvManager *venv;
    BatchRunner *batch;

    // Models/Views
    QStandardItemModel *requirementsModel;
    QTableView *requirementsView;
    QPlainTextEdit *logView;
    QProgressBar *progress;

    // Menus
    QMenu *recentLocalMenu;
    QMenu *recentWebMenu;

    // History widget
    MatrixHistory *historyWidget;

    // State
    QStringList historyRecentLocal;
    QStringList historyRecentWeb;

    // Settings
    int maxHistoryItems;     // -1=unlimited, 0 invalid, ≥1 valid
    QString appVersion;
};

/************** End of MainWindow.h **************************/
