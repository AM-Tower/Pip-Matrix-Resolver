#include <QTest>
#include "test_mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    int status = 0;
    status |= QTest::qExec(new TestMainWindow, argc, argv);
    return status;
}
