#ifndef MQTT_CONNECTION_MANAGER_H
#define MQTT_CONNECTION_MANAGER_H

#include <QWidget>
#include <QMqttClient>
#include <QLineEdit>
#include <QPushButton>

class MQTTConnectionManager : public QWidget {
	Q_OBJECT
public:
	MQTTConnectionManager(QWidget* parent = nullptr);
private:
	QMqttClient* m_pmqttClient = nullptr;

	QLineEdit* m_pleHost = nullptr;
	QLineEdit* m_plePort = nullptr;
	QPushButton* m_pbtnConnect = nullptr;
public slots:
	void onBtnConnectClick();
};

#endif MQTT_CONNECTION_MANAGER_H	
