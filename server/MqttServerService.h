#ifndef MQTT_SERVER_SERVICE_H
#define MQTT_SERVER_SERVICE_H

#include <QObject>
#include <QMqttClient>
#include <QJsonDocument>
#include <QJsonObject>

#include <general/DatabaseManager.h>

class MqttServerService : public QObject {
    Q_OBJECT
public:
    explicit MqttServerService(QObject* parent = nullptr);
    void connectToBroker(const QString& host, quint16 port);

private slots:
    void onConnected();
    void onMessageReceived(const QByteArray& message, const QMqttTopicName& topic);

private:
    QMqttClient* m_mqttClient = nullptr;
};

#endif