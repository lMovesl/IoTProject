#include <QCoreApplication>
#include "MqttServerService.h"

int main(int argc, char* argv[]) {
    //qputenv("QT_DEBUG_PLUGINS", QByteArray("1"));
    QCoreApplication a(argc, argv);
    MqttServerService service;
    service.connectToBroker("localhost", 1883);

    return a.exec();
}