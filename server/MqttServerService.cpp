#include "MqttServerService.h"
#include <QDebug>

MqttServerService::MqttServerService(QObject* parent) : QObject(parent) {
    m_mqttClient = new QMqttClient(this);

    connect(m_mqttClient, &QMqttClient::connected, this, &MqttServerService::onConnected);
    connect(m_mqttClient, &QMqttClient::messageReceived, this, &MqttServerService::onMessageReceived);
}

void MqttServerService::connectToBroker(const QString& host, quint16 port) {
    m_mqttClient->setHostname(host);
    m_mqttClient->setPort(port);
    m_mqttClient->connectToHost();
}

void MqttServerService::onConnected() {
    auto subscription = m_mqttClient->subscribe(QMqttTopicFilter("devices/+/data"));
    if (subscription) {
        qDebug() << "Сервер начал прослушивание канала 'devices/+/data'";
    }
}

void MqttServerService::onMessageReceived(const QByteArray& message, const QMqttTopicName& topic) {
    qDebug() << "Получены данные от:" << topic.name();
    if (!DatabaseManager::instance().open()) return;
    DatabaseManager::instance().processCombinedJson(topic.name(), message);
}