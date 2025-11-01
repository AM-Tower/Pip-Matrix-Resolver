/******************************************************************
 * File: MatrixUtility.h
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Provides utility functions for PipMatrixResolverQt.
 *   Handles requirements history, URL normalization,
 *   persistence, and view helpers.
 *
 * Version: 0.1
 * Date:    2025-10-31
 ******************************************************************/

#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QTableView>

class QStandardItemModel;

namespace MatrixUtility
{
/**************************************************************
 * Directory helpers
**************************************************************/
QString logsDir();
QString historyDir();

/**************************************************************
 * URL helpers
 **************************************************************/
QString normalizeRawUrl(const QString& url);

/**************************************************************
 * File helpers
 **************************************************************/
QStringList readTextFileLines(const QString& path);
void writeTableToModel(QStandardItemModel* model, const QStringList& lines);

/**************************************************************
 * View helpers
 **************************************************************/
void ensureViewScrollable(QTableView* view);

/**************************************************************
 * Network helpers
 **************************************************************/
bool downloadText(const QString& url, QByteArray& out);
}

/************** End of File.h **************/
