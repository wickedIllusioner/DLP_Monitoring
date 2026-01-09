#include <QApplication>
#include <QStyleFactory>
#include <QPalette>

#include "include/MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QApplication::setApplicationName("DLP Monitoring System");
    QApplication::setOrganizationName("DLP Labs");
    QApplication::setApplicationVersion("1.0.0");

    QApplication::setStyle(QStyleFactory::create("Fusion"));

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}