/******************************************************************
 * File: MatrixUtility.cpp
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Implements utility functions for PipMatrixResolverQt.
 *
 * Version: 0.1
 * Date:    2025-10-31
 ******************************************************************/

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

/******************************************************************
 * logsDir
 ******************************************************************/
QString MatrixUtility::logsDir()
{
    QDir dir("logs");
    if (!dir.exists()) dir.mkpath(".");
    return dir.absolutePath();
}

/******************************************************************
 * historyDir
 ******************************************************************/
QString MatrixUtility::historyDir()
{
    QDir dir("requirement-history");
    if (!dir.exists()) dir.mkpath(".");
    return dir.absolutePath();
}

/******************************************************************
 * normalizeRawUrl
 ******************************************************************/
QString MatrixUtility::normalizeRawUrl(const QString& url)
{
    const QUrl u(url);
    const QString host = u.host().toLower();
    if (host == "github.com") {
        const QStringList parts = u.path().split('/', Qt::SkipEmptyParts);
        if (parts.size() >= 5 && parts[2] == "blob") {
            const QString owner = parts[0];
            const QString repo  = parts[1];
            const QString branch = parts[3];
            QString rest = parts.mid(4).join('/');
            return QString("https://raw.githubusercontent.com/%1/%2/%3/%4")
                .arg(owner, repo, branch, rest);
        }
    }
    if (host == "raw.githubusercontent.com") {
        return url;
    }
    return url;
}

/******************************************************************
 * readTextFileLines
 ******************************************************************/
QStringList MatrixUtility::readTextFileLines(const QString& path)
{
    QStringList lines;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!f.atEnd()) {
            lines << QString::fromUtf8(f.readLine()).trimmed();
        }
        f.close();
    }
    return lines;
}

/******************************************************************
 * writeTableToModel
 ******************************************************************/
void MatrixUtility::writeTableToModel(QStandardItemModel* model,
                                      const QStringList& lines)
{
    if (!model) return;
    model->clear();
    model->setColumnCount(1);
    model->setHorizontalHeaderLabels({ QObject::tr("requirements.txt") });
    for (int i = 0; i < lines.size(); ++i) {
        auto* item = new QStandardItem(lines[i]);
        item->setEditable(false);
        model->setItem(i, 0, item);
    }
}

/******************************************************************
 * ensureViewScrollable
 ******************************************************************/
void MatrixUtility::ensureViewScrollable(QTableView *view)
{
    Q_ASSERT(view);
    if (!view || view->parent() == nullptr) {
        qWarning() << "ensureViewScrollable called with invalid view";
        return;
    }
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->resizeColumnsToContents();
}

/******************************************************************
 * downloadText
 ******************************************************************/
bool MatrixUtility::downloadText(const QString& url, QByteArray& out)
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
    QNetworkReply* reply = mgr.get(req);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError)
    {
        out = reply->readAll();
        reply->deleteLater();
        return true;
    }
    else
    {
        reply->deleteLater();
        return false;
    }
}

/************** End of File.cpp **************/
