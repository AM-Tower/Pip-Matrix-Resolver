#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QIcon>
#include <QDebug>
#include "MainWindow.h"
#include "Config.h"

#define SHOW_DEBUG 0

int main(int argc, char *argv[])
{
    // Set app version before MainWindow is constructed
    MainWindow::appVersion = "0.1.3"; // or any version you wan

    QCoreApplication::setOrganizationName(MainWindow::kOrganizationName);
    QCoreApplication::setApplicationName(MainWindow::kApplicationName);

    QApplication theApplication(argc, argv);

    // Register compiled resources
    Q_INIT_RESOURCE(PipMatrixResolverQt);

    // Enable icons in menus globally
    theApplication.setAttribute(Qt::AA_DontShowIconsInMenus, false);

    // Set application icon
    QApplication::setWindowIcon(QIcon(":/icons/icons/app.svg"));

    // Diagnostics
    DEBUG_MSG() << "[RESOURCE CHECK] :/icons/icons/open.svg exists:" << QFile::exists(":/icons/icons/open.svg");
    // Translation loading
    const QString languageCode = QLocale::system().name().split('_').first();
    auto loadTranslator = [&](const QString &baseName) -> bool
    {
        QTranslator *tr = new QTranslator(&theApplication);
        const QString qmFile = QString(":/translations/%1_%2.qm").arg(baseName, languageCode);
        if (tr->load(qmFile))
        {
            theApplication.installTranslator(tr);
            qDebug() << "Loaded translation:" << qmFile;
            return true;
        }
        delete tr;
        return false;
    };

    loadTranslator("PipMatrixResolverQt");
    loadTranslator("MatrixUtility");
    loadTranslator("MatrixHistory");

    MainWindow w;
    w.show();
    return theApplication.exec();
}
