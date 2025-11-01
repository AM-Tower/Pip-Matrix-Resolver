/******************************************************************
 * File: MainWindow.h
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Declares the MainWindow class for PipMatrixResolverQt.
 *   Provides menus, toolbar, requirements.txt loading (local & web),
 *   and integration with ResolverEngine, VenvManager, BatchRunner,
 *   MatrixHistory, and MatrixUtility.
 *
 * Version: 0.3
 * Date:    2025-10-31
 ******************************************************************/

#pragma once

#include "MatrixHistory.h"
#include <QMainWindow>
#include <QPointer>
#include <QStringList>
#include <QStackedWidget>

class QPlainTextEdit;
class QTableView;
class QProgressBar;
class QStandardItemModel;
class QMenu;
class QAction;
class ResolverEngine;
class VenvManager;
class BatchRunner;

/******************************************************************
 * @brief The MainWindow class
 * Provides the main application window, menus, toolbar,
 * requirements.txt loading, and integration with engines.
 ******************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    /**************************************************************
     * File menu actions
     **************************************************************/
    void openLocalRequirements();
    void fetchRequirementsFromUrl();
    void openMatrixHistory();
    void exitApp();

    /**************************************************************
     * Tools menu actions
     **************************************************************/
    void createOrUpdateVenv();
    void startResolve();
    void pauseResolve();
    void resumeResolve();
    void stopResolve();

    /**************************************************************
     * Batch menu actions
     **************************************************************/
    void runBatch();

    /**************************************************************
     * Logging and progress
     **************************************************************/
    void appendLog(const QString &line);
    void updateProgress(int percent);
    void showCompiledResult(const QString &path);

private:
    /**************************************************************
     * Setup helpers
     **************************************************************/
    void setupUi();
    void setupMenus();
    void setupToolbar();
    void bindSignals();

    /**************************************************************
     * Utility helpers
     **************************************************************/
    void applyToolsEnabled(bool enabled);

    /**************************************************************
     * UI elements
     **************************************************************/
    QTableView *requirementsView;
    QTableView *matrixView;
    QPlainTextEdit *logView;
    QProgressBar *progress;
    QStandardItemModel *requirementsModel;

    /**************************************************************
     * Engine components
     **************************************************************/
    QPointer<ResolverEngine> resolver;
    QPointer<VenvManager> venv;
    QPointer<BatchRunner> batch;

    /**************************************************************
     * Menus and actions
     **************************************************************/
    QMenu *toolsMenu;
    QAction *actionCreateVenv;
    QAction *actionResolveMatrix;
    QAction *actionPause;
    QAction *actionResume;
    QAction *actionStop;
    QMenu *recentWebMenu;
    QMenu *recentLocalMenu;

    /**************************************************************
     * State
     **************************************************************/
    bool hasValidRequirements;

    /**************************************************************
     * Lists mirror MatrixHistory state
     **************************************************************/
    QStringList historyRecentLocal;
    QStringList historyRecentWeb;

    /**************************************************************
     * Stacked Widget
     **************************************************************/
    QStackedWidget *stacked;
    QWidget *mainPage;
    MatrixHistory *historyPage;

    // Helper to rebuild menus
    void refreshRecentMenus();

};

/************** End of File.h **************/
