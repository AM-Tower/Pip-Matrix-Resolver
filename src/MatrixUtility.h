/****************************************************************
 * @file MatrixUtility.h
 * @brief Utility functions for PipMatrixResolverQt.
 *
 * @author Jeffrey
 * @version 0.4
 * @date    2025-11-01
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Provides helpers for reading requirements files, writing them
 * into models, validating entries, ensuring view scrollability,
 * and downloading text resources.
 ***************************************************************/

#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>

class QStandardItemModel;
class QTableView;

namespace MatrixUtility
{
/************************************************************
     * @brief Return logs directory, create if missing.
     ************************************************************/
QString logsDir();

/************************************************************
     * @brief Return history directory, create if missing.
     ************************************************************/
QString historyDir();

/************************************************************
     * @brief Normalize GitHub blob URLs to raw URLs.
     ************************************************************/
QString normalizeRawUrl(const QString &url);

/************************************************************
     * @brief Read file lines, strip blanks and comments.
     ************************************************************/
QStringList readTextFileLines(const QString &path);

/************************************************************
     * @brief Write requirements into a one-column model.
     ************************************************************/
void writeTableToModel(QStandardItemModel *model,
                       const QStringList &lines);

/************************************************************
     * @brief Ensure scrollbars and column sizing for a view.
     ************************************************************/
void ensureViewScrollable(QTableView *view);

/************************************************************
     * @brief Download text from local or remote URL.
     ************************************************************/
bool downloadText(const QString &url, QByteArray &out);

/************************************************************
     * @brief Validate requirements quickly.
     ************************************************************/
bool validateRequirements(const QStringList &lines);

/************************************************************
     * @brief Validate with detailed error reporting.
     ************************************************************/
bool validateRequirementsWithErrors(const QStringList &lines,
                                    QStringList &errors);
}

/************** End of MatrixUtility.h **************************/
