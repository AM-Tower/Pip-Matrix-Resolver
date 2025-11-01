/****************************************************************
 * File: MatrixHistory.h
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Provides a tabbed UI for managing requirements history inside
 *   the MainWindow. Tabs: Local and Web addresses (separate tabs),
 *   Requirements preview, and Settings. Emits signals to import
 *   a snapshot (local or web) or exit back to Main tab.
 *
 * Version: 0.3
 * Date:    2025-11-01
 ****************************************************************/

#pragma once

#include <QWidget>
#include <QStringList>

class QTabWidget;
class QTableView;
class QPlainTextEdit;
class QSpinBox;
class QPushButton;
class QStandardItemModel;

/****************************************************************
 * @brief The MatrixHistory class
 * Manages requirements history via tabs:
 * - Local tab: table of local files
 * - Web tab: table of web URLs
 * - Requirements tab: preview of selected requirements
 * - Settings tab: history size and controls
 *
 * Signals:
 * - localHistoryImported(path)
 * - webHistoryImported(url)
 * - historyCleared()
 * - exitRequested()
 ****************************************************************/
class MatrixHistory : public QWidget
{
    Q_OBJECT

public:
    explicit MatrixHistory( QWidget *parent = nullptr );

signals:
    /************************************************************
     * Events emitted to MainWindow
     ************************************************************/
    void historyCleared();
    void localHistoryImported( const QString &path );
    void webHistoryImported( const QString &url );
    void exitRequested();

private slots:
    /************************************************************
     * User actions mapped to signals
     ************************************************************/
    void importLocalSelected();
    void importWebSelected();
    void backToMain();
    void maxItemsChanged( int value );

private:
    /************************************************************
     * Internal helpers
     ************************************************************/
    void setupUi();
    void loadSettings();
    void saveSettings() const;
    void refreshLocalTab();
    void refreshWebTab();
    void refreshRequirementsTab( const QString &path );
    void clearHistory();

private:
    /************************************************************
     * Widgets
     ************************************************************/
    QTabWidget *tabs;

    // Local tab
    QWidget         *localTab;
    QTableView      *localView;
    QPushButton     *importLocalButton;
    QStandardItemModel *localModel;

    // Web tab
    QWidget         *webTab;
    QTableView      *webView;
    QPushButton     *importWebButton;
    QStandardItemModel *webModel;

    // Requirements preview tab
    QWidget         *reqTab;
    QPlainTextEdit  *requirementsPreview;

    // Settings tab
    QWidget         *settingsTab;
    QSpinBox        *maxItemsSpin;
    QPushButton     *clearButton;
    QPushButton     *backButton;

    /************************************************************
     * Data
     ************************************************************/
    QStringList m_recentLocal;
    QStringList m_recentWeb;
    int         m_maxItems;
};
/************** End of MatrixHistory.h *************************/
