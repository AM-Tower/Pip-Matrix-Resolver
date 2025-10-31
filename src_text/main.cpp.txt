#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QFile>
#include <QDebug>
#include "MainWindow.h"

// Include the generated header for your resource file
// #include <QtGlobal> void initResources() { Q_INIT_RESOURCE(PipMatrixResolverQt); }

int main(int argc, char *argv[])
{
    QApplication theApplication(argc, argv);
    // Enable icons in menus globally
    theApplication.setAttribute(Qt::AA_DontShowIconsInMenus, false);

    QApplication::setWindowIcon(QIcon(":/resources/icons/app.svg"));

    // --- Resource diagnostics (optional) ---
    auto checkIcon = [](const char* rpath)
    {
        qDebug() << "[RESOURCE CHECK]" << rpath
                 << "exists:" << QFile::exists(rpath)
                 << "icon.isNull:" << QIcon(rpath).isNull();
    };
    checkIcon(":/resources/icons/open.svg");

    // --- Translation load from resource ---
    QTranslator translator;
    const QString languageCode = QLocale::system().name().split('_').first();
    const QString qmFileName = QString("PipMatrixResolverQt_%1.qm").arg(languageCode);
    const QString resDir = QString(":/translations");
    const QString resPath = QString("%1/%2").arg(resDir, qmFileName);
    if (QFile::exists(resPath))
    {
        if (translator.load(qmFileName, resDir))
        {
            theApplication.installTranslator(&translator);
        }
        else
        {
            qWarning() << "TRANSLATION ERROR: Resource exists but load failed for"
                       << languageCode << "at" << resPath;
        }
    }
    else
    {
        qWarning() << "TRANSLATION ERROR: Resource not found at" << resPath;
    }
    MainWindow w;
    w.show();
    return theApplication.exec();
}
