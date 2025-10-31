#include <QtTest/QtTest>
#include "MainWindow.h"

class TestMainWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase() 
    {
        // Runs before all tests
    }

    void testFetchRequirementsAction() {
        MainWindow w;
        QAction *action = w.findChild<QAction*>("actionFetchRequirements");
        QVERIFY(action != nullptr);
    }

    void cleanupTestCase() {
        // Runs after all tests
    }
};

#include "test_mainwindow.moc"
