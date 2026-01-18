#ifndef MQTT_CONNECTION_MANAGER_H
#define MQTT_CONNECTION_MANAGER_H

#include <QWidget>
#include <QMqttClient>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>

class MQTTConnectionManager : public QWidget {
	Q_OBJECT
public:
	MQTTConnectionManager(QWidget* parent = nullptr);
private:
	QMqttClient* m_pmqttClient = nullptr;
	quint16 m_uiPort;
	QMqttClient::ProtocolVersion m_protocolVersion = QMqttClient::MQTT_3_1;
	bool m_bWebSockets = false;
	bool m_bSecure = false;

	QLineEdit* m_pleHost = nullptr;
	QLineEdit* m_pleTopicName = nullptr;
	QSpinBox* m_psbPort = nullptr;
	QComboBox* m_pcbProtocol = nullptr;
	QPushButton* m_pbtnConnect = nullptr;
	QPushButton* m_pbtnTopicSubscribe = nullptr;
	QCheckBox* m_pchbSecure = nullptr;
	//QCheckBox* m_pchbWebSockets = nullptr;
	QPlainTextEdit* m_ppteLogMessages = nullptr;
public slots:
	void onBtnConnectClick();
	void onBtnSubscribeClick();
	void onMessageReceived(const QByteArray& message, const QMqttTopicName& topic);
	void onDisconnectBroker();
	void setClientPort(int port);
};

#endif MQTT_CONNECTION_MANAGER_H	
