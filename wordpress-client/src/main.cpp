#include "ui/main-window.hpp"

#include <QApplication>
#include <QIcon>

extern "C" {
#include <curl/curl.h>
}

int main(int argc, char* argv[])
{
    // Initialize libcurl globally before any threads start
    curl_global_init(CURL_GLOBAL_DEFAULT);

    QApplication app(argc, argv);
    app.setApplicationName("WordPress Desktop Client");
    app.setOrganizationName("wp-desktop-client");
    app.setApplicationVersion("1.0.0");

    wpclient::MainWindow window;
    window.show();

    int ret = app.exec();

    curl_global_cleanup();
    return ret;
}
