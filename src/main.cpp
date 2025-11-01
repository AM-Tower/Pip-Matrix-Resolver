#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QDebug>
#include <QIcon>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    if (true)
    {
        QApplication app(argc, argv);
        MainWindow w;

        w.addDummyHistory();   // safe, public call

        w.show();
        return app.exec();
    }
    else
    {
        QApplication theApplication(argc, argv);

        // Register compiled resources
        Q_INIT_RESOURCE(PipMatrixResolverQt);

        // Enable icons in menus globally
        theApplication.setAttribute(Qt::AA_DontShowIconsInMenus, false);

        // Set application icon
        QApplication::setWindowIcon(QIcon(":/icons/icons/app.svg"));

        // Diagnostics
        qDebug() << "[RESOURCE CHECK] :/icons/icons/open.svg exists:" << QFile::exists(":/icons/icons/open.svg");

        // Translation loading
        const QString languageCode = QLocale::system().name().split('_').first();
        auto loadTranslator = [&](const QString &baseName) -> bool {
            QTranslator *tr = new QTranslator(&theApplication);
            const QString qmFile = QString(":/translations/%1_%2.qm").arg(baseName, languageCode);
            if (tr->load(qmFile)) {
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
}
