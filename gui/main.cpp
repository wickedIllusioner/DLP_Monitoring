#include "include/MainWindow.h"
#include "include/LoginDialog.h"
#include <QApplication>
#include <QMessageBox>
#include <QObject>
#include <QStyleFactory>


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    LoginDialog loginDialog;

    if (loginDialog.exec() != QDialog::Accepted) {
        return 0;
    }

    QApplication::setApplicationName("DLP Monitoring System");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}