#include "MqttServerService.h"
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
#include <QFile>
#include <QMetaEnum>

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

void MqttServerService::connectUsingSettings() {
    // Путь к файлу server_config.ini в папке с исполняемым файлом сервера
    QString configPath = QCoreApplication::applicationDirPath() + "/server_config.ini";
    QSettings settings(configPath, QSettings::IniFormat);

    // Если файла конфигурации не существует, создаем его с дефолтными параметрами
    if (!QFile::exists(configPath)) {
        settings.beginGroup("MQTT");
        settings.setValue("host", "localhost");
        settings.setValue("port", 1883);
        settings.setValue("protocol", 1); // 0 = MQTT_3_1, 1 = MQTT_3_1_1, 2 = MQTT_5_0
        settings.endGroup();
        settings.sync();
        qDebug() << "Создан дефолтный файл конфигурации по пути:" << configPath;
    }

    // Читаем настройки
    settings.beginGroup("MQTT");
    QString host = settings.value("host", "localhost").toString();
    quint16 port = static_cast<quint16>(settings.value("port", 1883).toUInt());
    int protocolIdx = settings.value("protocol", 1).toInt();
    settings.endGroup();

    // Настраиваем клиент
    m_mqttClient->setHostname(host);
    m_mqttClient->setPort(port);

    // Маппинг версии протокола
    QMqttClient::ProtocolVersion protocol = QMqttClient::MQTT_3_1_1;
    if (protocolIdx == 0) protocol = QMqttClient::MQTT_3_1;
    else if (protocolIdx == 2) protocol = QMqttClient::MQTT_5_0;
    m_mqttClient->setProtocolVersion(protocol);

    auto protocolStr = QMetaEnum::fromType<QMqttClient::ProtocolVersion>().valueToKey(protocol);
    qDebug() << QString("Попытка подключения к брокеру [%1:%2] с протоколом %3...")
        .arg(host).arg(port).arg(protocolStr);

    m_mqttClient->connectToHost();
}

void MqttServerService::connectToBroker(const QString& host, quint16 port) {
    m_mqttClient->setHostname(host);
    m_mqttClient->setPort(port);
    m_mqttClient->connectToHost();
}

void MqttServerService::onConnected() {
    auto subscription = m_mqttClient->subscribe(QMqttTopicFilter("devices/+/data"));
    if (subscription) {
        qDebug() << "Сервер успешно подключен и начал прослушивание канала 'devices/+/data'";
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