/****************************************************************
 * @file MatrixUtility.cpp
 * @brief Implementation of utility functions for
 *        PipMatrixResolverQt.
 *
 * @author Jeffrey
 * @version 0.4
 * @date    2025-11-01
 * @section License Unlicensed, MIT, or any.
 * @section DESCRIPTION
 * Implements helpers for reading requirements files, writing
 * them into models, validating entries, ensuring view
 * scrollability, and downloading text resources.
 ***************************************************************/

#include "MatrixUtility.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QScrollBar>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QRegularExpression>
#include <QTableView>
#include <QDebug>

/************************************************************
 * @brief Return logs directory, create if missing.
 ************************************************************/
QString MatrixUtility::logsDir()
{
    QDir dir("logs");
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

/************************************************************
 * @brief Return history directory, create if missing.
 ************************************************************/
QString MatrixUtility::historyDir()
{
    QDir dir("requirement-history");
    if (!dir.exists())
    {
        dir.mkpath(".");
    }
    return dir.absolutePath();
}

/************************************************************
 * @brief Normalize GitHub blob URLs to raw URLs.
 ************************************************************/
QString MatrixUtility::normalizeRawUrl(const QString &url)
{
    const QUrl u(url);
    const QString host = u.host().toLower();
    if (host == "github.com")
    {
        const QStringList parts = u.path().split('/', Qt::SkipEmptyParts);
        if (parts.size() >= 5 && parts[2] == "blob")
        {
            const QString owner = parts[0];
            const QString repo  = parts[1];
            const QString branch = parts[3];
            QString rest = parts.mid(4).join('/');
            return QString("https://raw.githubusercontent.com/%1/%2/%3/%4")
                .arg(owner, repo, branch, rest);
        }
    }
    return url;
}

/************************************************************
 * @brief Read file lines, strip blanks and comments.
 ************************************************************/
QStringList MatrixUtility::readTextFileLines(const QString &path)
{
    QStringList lines;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        while (!f.atEnd())
        {
            QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line.isEmpty()) continue;
            if (line.startsWith('#')) continue;
            lines << line;
        }
        f.close();
    }
    return lines;
}

/************************************************************
 * @brief Write requirements into a one-column model.
 ************************************************************/
void MatrixUtility::writeTableToModel(QStandardItemModel *model,
                                      const QStringList &lines)
{
    if (!model) return;

    model->clear();
    model->setColumnCount(1);
    model->setHorizontalHeaderLabels(
        { QObject::tr("requirements.txt") });

    for (const QString &line : lines)
    {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) continue;
        auto *item = new QStandardItem(trimmed);
        item->setEditable(false);
        model->appendRow(item);
    }
}

/************************************************************
 * @brief Download text from local or remote URL.
 ************************************************************/
bool MatrixUtility::downloadText(const QString &url, QByteArray &out)
{
    const QUrl u(url);
    if (u.isLocalFile() || QFileInfo::exists(url))
    {
        QFile f(u.isLocalFile() ? u.toLocalFile() : url);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            out = f.readAll();
            f.close();
            return true;
        }
        return false;
    }

    QNetworkAccessManager mgr;
    QNetworkRequest req{ QUrl(url) };
    QEventLoop loop;
    QNetworkReply *reply = mgr.get(req);
    QObject::connect(reply, &QNetworkReply::finished,
                     &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError)
    {
        out = reply->readAll();
        reply->deleteLater();
        return true;
    }
    reply->deleteLater();
    return false;
}

/************************************************************
 * @brief Validate requirements quickly.
 ************************************************************/
bool MatrixUtility::validateRequirements(const QStringList &lines)
{
    QStringList errors;
    return validateRequirementsWithErrors(lines, errors);
}

/************************************************************
 * @brief Validate with detailed error reporting.
 ************************************************************/
bool MatrixUtility::validateRequirementsWithErrors(
    const QStringList &lines,
    QStringList &errors)
{
    errors.clear();
    if (lines.isEmpty())
    {
        errors << "Empty input: no lines to validate.";
        return false;
    }

    static const QRegularExpression re(R"(^([A-Za-z0-9_.-]+)(\[[A-Za-z0-9_.\-,\s]+\])?\s*(?:([=><!~]{1,2})\s*([^\s#;]+))?(?:\s*;[^#]+)?$)");

    bool anyMeaningful = false;
    for (int i = 0; i < lines.size(); ++i)
    {
        const QString raw = lines.at(i);
        const QString trimmed = raw.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith('#')) continue;
        anyMeaningful = true;
        if (!re.match(trimmed).hasMatch())
        {
            errors << QString("Line %1 failed: \"%2\"")
            .arg(i + 1).arg(raw);
        }
    }
    if (!anyMeaningful)
    {
        errors << "No meaningful requirement lines found.";
        return false;
    }
    return errors.isEmpty();
}

/************** End of MatrixUtility.cpp *************************/
