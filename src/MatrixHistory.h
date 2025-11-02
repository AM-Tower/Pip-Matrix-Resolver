/****************************************************************
 * @file MatrixHistory.h
 * @brief Declares the MatrixHistory widget for
 *        PipMatrixResolverQt.
 *
 * @author Jeffrey
 * @version 0.6
 * @date    2025-11-01
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Provides a widget to display and manage recent requirements
 * history (local and web). Emits signals when history changes.
 ***************************************************************/

#pragma once

#include <QWidget>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

/****************************************************************
 * @class MatrixHistory
 * @brief Widget for displaying and managing requirements
 *        history.
 ***************************************************************/
class MatrixHistory : public QWidget
{
    Q_OBJECT

public:
    explicit MatrixHistory(QWidget *parent = nullptr);
    ~MatrixHistory();

    /************************************************************
     * @brief Set the history list to display.
     * @param history Combined list of local and web entries.
     ************************************************************/
    void setHistory(const QStringList &history);

    /************************************************************
     * @brief Get the current history list.
     * @return QStringList of history entries.
     ************************************************************/
    QStringList history() const;

signals:
    /************************************************************
     * @brief Emitted when the history list changes.
     ************************************************************/
    void historyChanged();

private:
    QListWidget *listWidget;
    QStringList currentHistory;

    void rebuildList();
};

/************** End of MatrixHistory.h **************************/
