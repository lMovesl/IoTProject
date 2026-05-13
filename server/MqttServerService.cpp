#include "MqttServerService.h"
#include <QDebug>

MqttServerService::MqttServerService(QObject* parent) : QObject(parent) {
    m_mqttClient = new QMqttClient(this);

    connect(m_mqttClient, &QMqttClient::connected, this, &MqttServerService::onConnected);
    connect(m_mqttClient, &QMqttClient::messageReceived, this, &MqttServerService::onMessageReceived);

    QObject::connect(&DatabaseManager::instance(), &DatabaseManager::anomalyDetected,
        [this](const QString& uid, const QString& devName, const QString& key, double val, const QString& type) {

            QString alertTopic = QString("devices/%1/alerts").arg(uid);

            QJsonObject root;
            root["sensor"] = key;
            root["device_name"] = devName; 
            root["type"] = type;
            root["value"] = val;

            publishAlert(alertTopic, QJsonDocument(root).toJson());
        });
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
    QString topicName = topic.name();
    qDebug() << "Получены данные от топика:" << topicName;

    QStringList parts = topicName.split('/');

    if (parts.size() >= 2) {
        QString macAddress = parts.at(1); 

        if (!DatabaseManager::instance().open()) return;

        DatabaseManager::instance().processCombinedJson(macAddress, message);

        int deviceId = DatabaseManager::instance().getDeviceIdByMac(macAddress);
        if (deviceId != -1) {
            DatabaseManager::instance().updateLastSeen(deviceId);
        }
    }
    else {
        qWarning() << "Неверный формат топика:" << topicName;
    }
}

void MqttServerService::publishAlert(const QString& topic, const QString& message) {
    if (m_mqttClient->state() == QMqttClient::Connected) {
        m_mqttClient->publish(QMqttTopicName(topic), message.toUtf8());
    }
}