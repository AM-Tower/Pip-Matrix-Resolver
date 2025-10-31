#pragma once

#include <QMainWindow>
#include <QPointer>

class QPlainTextEdit;
class QTableView;
class QProgressBar;
class ResolverEngine;
class VenvManager;
class BatchRunner;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    // File
    void openLocalRequirements();
    void fetchRequirementsFromUrl();
    void exitApp();

    // Tools
    void createOrUpdateVenv();
    void startResolve();
    void pauseResolve();
    void resumeResolve();
    void stopResolve();

    // Batch
    void runBatch();

    // Logging
    void appendLog(const QString &line);
    void updateProgress(int percent);
    void showCompiledResult(const QString &path);

private:
    void setupUi();
    void setupMenus();
    void setupToolbar();
    void bindSignals();

    QTableView *requirementsView;
    QTableView *matrixView;
    QPlainTextEdit *logView;
    QProgressBar *progress;

    QPointer<ResolverEngine> resolver;
    QPointer<VenvManager> venv;
    QPointer<BatchRunner> batch;
};
