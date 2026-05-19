#include <QCoreApplication>
#include "MqttServerService.h"

int main(int argc, char* argv[]) {
    std::setlocale(LC_ALL, "RU");
    QCoreApplication a(argc, argv);

    MqttServerService service;

    service.connectUsingSettings();

    return a.exec();
}