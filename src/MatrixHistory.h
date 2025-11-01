/******************************************************************
 * File: MatrixHistory.h
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Provides a tabbed UI for managing requirements history.
 *   Tabs: Local, Web, Requirements, Settings.
 *   Emits signals to import a snapshot or exit back to MainWindow.
 *
 * Version: 0.2
 * Date:    2025-10-31
 ******************************************************************/

#pragma once

#include <QWidget>
#include <QStringList>

class QTabWidget;
class QTableView;
class QPlainTextEdit;
class QSpinBox;
class QPushButton;

/**************************************************************
 * @brief The MatrixHistory class
 * Provides tabbed history for local and web requirements.
 **************************************************************/
class MatrixHistory : public QWidget
{
    Q_OBJECT
public:
    explicit MatrixHistory(QWidget *parent = nullptr);

signals:
    /// Emitted when the user clears all history
    void historyCleared();

    /// Emitted when the user chooses a local snapshot to import.
    void localHistoryImported(const QString &path);

    /// Emitted when the user chooses a web snapshot to import.
    void webHistoryImported(const QString &url);

    /// Emitted when the user clicks Back to return to MainWindow.
    void exitRequested();

private slots:
    void importLocalSelected();
    void importWebSelected();
    void backToMain();
    void maxItemsChanged(int value);

private:
    void clearHistory();   // private helper, not a slot
    void setupUi();
    void loadSettings();
    void saveSettings() const;
    void refreshLocalTab();
    void refreshWebTab();
    void refreshRequirementsTab(const QString &path);

    QTabWidget *tabs;

    // Local tab
    QTableView *localView;
    QPushButton *importLocalButton;

    // Web tab
    QTableView *webView;
    QPushButton *importWebButton;

    // Requirements preview
    QPlainTextEdit *requirementsPreview;

    // Settings
    QSpinBox *maxItemsSpin;
    QPushButton *backButton;

    QStringList m_recentLocal;
    QStringList m_recentWeb;
    int m_maxItems;
};

/************** End of File.h **************/
