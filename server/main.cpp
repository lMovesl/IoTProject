#include <QCoreApplication>
#include "MqttServerService.h"

int main(int argc, char* argv[]) {
    
    std::setlocale(LC_ALL, "RU");
    //qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));
    QCoreApplication a(argc, argv); //todo increase lifetime mqtt
    MqttServerService service;
    service.connectToBroker("localhost", 1883);

    return a.exec();
}