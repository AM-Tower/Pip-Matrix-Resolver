/****************************************************************
 * @file MatrixHistory.cpp
 * @brief Implements the MatrixHistory widget for
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

#include "MatrixHistory.h"

#include <QListWidget>
#include <QVBoxLayout>

/****************************************************************
 * @brief Constructor.
 ***************************************************************/
MatrixHistory::MatrixHistory(QWidget *parent)
    : QWidget(parent),
    listWidget(new QListWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(listWidget);
    setLayout(layout);
}

/****************************************************************
 * @brief Destructor.
 ***************************************************************/
MatrixHistory::~MatrixHistory() = default;

/****************************************************************
 * @brief Set the history list to display.
 ***************************************************************/
void MatrixHistory::setHistory(const QStringList &history)
{
    if (history == currentHistory)
    {
        return;
    }

    currentHistory = history;
    rebuildList();
    emit historyChanged();
}

/****************************************************************
 * @brief Get the current history list.
 ***************************************************************/
QStringList MatrixHistory::history() const
{
    return currentHistory;
}

/****************************************************************
 * @brief Rebuild the list widget from current history.
 ***************************************************************/
void MatrixHistory::rebuildList()
{
    listWidget->clear();
    for (int i = 0; i < currentHistory.size(); ++i)
    {
        listWidget->addItem(currentHistory.at(i));
    }
}

/************** End of MatrixHistory.cpp **************************/
