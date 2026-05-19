#include "ui/main-window.hpp"
#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("WordPress Desktop Client");
    app.setOrganizationName("wp-desktop-client");
    app.setApplicationVersion("1.0.0");

    wpclient::MainWindow window;
    window.show();

    return app.exec();
}
