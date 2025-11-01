/******************************************************************
 * File: MainWindow.h
 * Author: Jeffrey Scott Flesher
 * Description:
 *   Main application window for PipMatrixResolverQt.
 *   Provides menus, log view, progress bar, and integration with
 *   ResolverEngine, VenvManager, BatchRunner, MatrixHistory, and
 *   MatrixUtility.
 *
 * Version: 0.5
 * Date:    2025-11-01
 ******************************************************************/

#pragma once

#include <QMainWindow>
#include <QStringList>

class QPlainTextEdit;
class QProgressBar;
class QTableView;
class QStandardItemModel;
class MatrixHistory;
class ResolverEngine;
class VenvManager;
class BatchRunner;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void openLocalRequirements();
    void fetchRequirementsFromUrl();
    void openMatrixHistory();
    void createOrUpdateVenv();
    void startResolve();
    void pauseResolve();
    void resumeResolve();
    void stopResolve();
    void runBatch();
    void showAboutBox();
    void showReadmeDialog();
    void exitApp();

    // internal helpers
    void appendLog(const QString &line);
    void updateProgress(int percent);
    void showCompiledResult(const QString &path);

private:
    void bindSignals();
    void applyToolsEnabled(bool enabled);
    void refreshRecentMenus();

    Ui::MainWindow *ui;

    // Core components
    ResolverEngine *resolver;
    VenvManager *venv;
    BatchRunner *batch;

    // Models and views
    QStandardItemModel *requirementsModel;
    QTableView *requirementsView;
    QTableView *matrixView;
    QPlainTextEdit *logView;
    QProgressBar *progress;
    MatrixHistory *historyWidget;

    // Recent history
    QStringList historyRecentLocal;
    QStringList historyRecentWeb;
    QMenu *recentLocalMenu {nullptr};
    QMenu *recentWebMenu {nullptr};

    // State
    bool hasValidRequirements;
};
