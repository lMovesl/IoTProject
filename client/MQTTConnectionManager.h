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
#include <QDialog>

class MQTTConnectionManager : public QDialog {
	Q_OBJECT
public:
	MQTTConnectionManager(QMqttClient* client, QWidget* parent = nullptr);
	QMqttClient* getMqttClient() const;
private:
	QMqttClient* m_pmqttClient = nullptr;
	quint16 m_uiPort;
	QMqttClient::ProtocolVersion m_protocolVersion = QMqttClient::MQTT_3_1;
	bool m_bWebSockets = false;
	bool m_bSecure = false;

	QLineEdit* m_pleHost = nullptr;
	QSpinBox* m_psbPort = nullptr;
	QComboBox* m_pcbProtocol = nullptr;
	QPushButton* m_pbtnConnect = nullptr;
	QPlainTextEdit* m_ppteLogMessages = nullptr;

	void loadSettingsFromClient(); // Метод для загрузки конфигурации
	void saveSettings();
private slots:
	void updateUiStates();
public slots:
	void onBtnConnectClick();
	void setClientPort(int port);
signals:
	void subscriptionMessageReceive(QMqttMessage);
};

#endif //MQTT_CONNECTION_MANAGER_H	
