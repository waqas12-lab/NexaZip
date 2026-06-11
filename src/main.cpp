#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("NexaZip");
    QApplication::setApplicationVersion("3.2.0");
    QApplication::setOrganizationName("NexaZip");
    QApplication::setWindowIcon(QIcon(":/icons/app.svg"));

    QFile style(":/themes/light.qss");
    if (style.open(QIODevice::ReadOnly | QIODevice::Text)) {
        app.setStyleSheet(QString::fromUtf8(style.readAll()));
    }

    MainWindow window;
    window.show();
    window.raise();
    window.activateWindow();

    return app.exec();
}
