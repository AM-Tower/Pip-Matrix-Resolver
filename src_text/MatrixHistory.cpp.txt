/******************************************************************
 * File: MatrixHistory.cpp
 * Author: Jeffrey Scott Flesher, Microsoft Copilot
 * Description:
 *   Implements MatrixHistory with tabbed UI for Local and Web
 *   requirements history. Provides import, clear, and settings.
 *
 * Version: 0.2
 * Date:    2025-10-31
 ******************************************************************/

#include "MatrixHistory.h"

#include <QTabWidget>
#include <QTableView>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QHeaderView>
#include <QSettings>
#include <QStandardItemModel>
#include <QFile>
#include <utility>   // for std::as_const

/******************************************************************
 * Constructor
 ******************************************************************/
MatrixHistory::MatrixHistory(QWidget *parent)
    : QWidget(parent),
    tabs(new QTabWidget(this)),
    localView(new QTableView(this)),
    importLocalButton(new QPushButton(tr("Import Local"), this)),
    webView(new QTableView(this)),
    importWebButton(new QPushButton(tr("Import Web"), this)),
    requirementsPreview(new QPlainTextEdit(this)),
    maxItemsSpin(new QSpinBox(this)),
    backButton(new QPushButton(tr("Back"), this)),
    m_maxItems(10)
{
    setupUi();
    loadSettings();
}

/******************************************************************
 * setupUi
 ******************************************************************/
void MatrixHistory::setupUi()
{
    // Local tab
    QWidget *localTab = new QWidget(this);
    QVBoxLayout *localLayout = new QVBoxLayout(localTab);
    localLayout->addWidget(localView);
    localLayout->addWidget(importLocalButton);
    tabs->addTab(localTab, tr("Local"));

    // Web tab
    QWidget *webTab = new QWidget(this);
    QVBoxLayout *webLayout = new QVBoxLayout(webTab);
    webLayout->addWidget(webView);
    webLayout->addWidget(importWebButton);
    tabs->addTab(webTab, tr("Web"));

    // Requirements preview tab
    QWidget *reqTab = new QWidget(this);
    QVBoxLayout *reqLayout = new QVBoxLayout(reqTab);
    requirementsPreview->setReadOnly(true);
    reqLayout->addWidget(requirementsPreview);
    tabs->addTab(reqTab, tr("Requirements"));

    // Settings tab
    QWidget *settingsTab = new QWidget(this);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsTab);
    maxItemsSpin->setRange(1, 100);
    settingsLayout->addWidget(maxItemsSpin);
    settingsLayout->addWidget(backButton);
    tabs->addTab(settingsTab, tr("Settings"));

    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabs);
    setLayout(mainLayout);

    // Connections
    connect(importLocalButton, &QPushButton::clicked,
            this, &MatrixHistory::importLocalSelected);
    connect(importWebButton, &QPushButton::clicked,
            this, &MatrixHistory::importWebSelected);
    connect(backButton, &QPushButton::clicked,
            this, &MatrixHistory::backToMain);
    connect(maxItemsSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &MatrixHistory::maxItemsChanged);
}
/******************************************************************
 * importLocalSelected
 ******************************************************************/
void MatrixHistory::importLocalSelected()
{
    // TODO: replace with actual selection from localView
    QString path = "dummy_local.txt";

    // Update preview tab
    requirementsPreview->setPlainText(
        tr("Preview of local requirements from: %1").arg(path));

    emit localHistoryImported(path);
}

/******************************************************************
 * importWebSelected
 ******************************************************************/
void MatrixHistory::importWebSelected()
{
    // TODO: replace with actual selection from webView
    QString url = "http://example.com/requirements.txt";

    // Update preview tab
    requirementsPreview->setPlainText(
        tr("Preview of web requirements from: %1").arg(url));

    emit webHistoryImported(url);
}

/******************************************************************
 * backToMain
 ******************************************************************/
void MatrixHistory::backToMain()
{
    emit exitRequested();
}

/******************************************************************
 * loadSettings / saveSettings
 ******************************************************************/
void MatrixHistory::loadSettings()
{
    QSettings s("PipMatrixResolverQt", "MatrixHistory");
    m_maxItems    = s.value("recent/maxItems", 20).toInt();
    m_recentLocal = s.value("recent/local").toStringList();
    m_recentWeb   = s.value("recent/web").toStringList();
    maxItemsSpin->setValue(m_maxItems);

    refreshLocalTab();
    refreshWebTab();
}

/******************************************************************
 * saveSettings
 ******************************************************************/
void MatrixHistory::saveSettings() const
{
    QSettings s("PipMatrixResolverQt", "MatrixHistory");
    s.setValue("recent/maxItems", m_maxItems);
    s.setValue("recent/local", m_recentLocal);
    s.setValue("recent/web", m_recentWeb);
}

/******************************************************************
 * refreshLocalTab
 ******************************************************************/
void MatrixHistory::refreshLocalTab()
{
    auto *model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({ tr("Local Files") });

    for (const QString &path : std::as_const(m_recentLocal))
    {
        auto *item = new QStandardItem(path);
        item->setEditable(false);
        model->appendRow(item);
    }

    localView->setModel(model);
    localView->horizontalHeader()->setStretchLastSection(true);
}

/******************************************************************
 * refreshWebTab
 ******************************************************************/
void MatrixHistory::refreshWebTab()
{
    auto *model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels({ tr("Web URLs") });

    for (const QString &url : std::as_const(m_recentWeb))
    {
        auto *item = new QStandardItem(url);
        item->setEditable(false);
        model->appendRow(item);
    }

    webView->setModel(model);
    webView->horizontalHeader()->setStretchLastSection(true);
}

/******************************************************************
 * refreshRequirementsTab
 ******************************************************************/
void MatrixHistory::refreshRequirementsTab(const QString &path)
{
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        requirementsPreview->setPlainText(
            QString::fromUtf8(f.readAll()));
        f.close();
    }
}

/******************************************************************
 * clearHistory
 ******************************************************************/
void MatrixHistory::clearHistory()
{
    m_recentLocal.clear();
    m_recentWeb.clear();

    refreshLocalTab();
    refreshWebTab();

    saveSettings();

    emit historyCleared();   // emit the signal here
}

/******************************************************************
 * maxItemsChanged
 ******************************************************************/
void MatrixHistory::maxItemsChanged(int value)
{
    m_maxItems = value;
    saveSettings();
}

/************** End of MatrixHistory.cpp **************************/
