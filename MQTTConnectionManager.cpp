#include "MQTTConnectionManager.h"

#include <QLabel>
#include <QGridLayout>
#include <QDateTime>
#include <QMessageBox>
#include <QGroupBox>

MQTTConnectionManager::MQTTConnectionManager(QWidget* parent) : QWidget(parent) {
	m_pmqttClient = new QMqttClient(this);
	QLabel* lblHost = new QLabel("Host", this);
	QLabel* lblPort = new QLabel("Port", this);
	QLabel* lblTopicName = new QLabel("Topic name", this);
	QLabel* lblLogMessages = new QLabel("Log messages", this);
	QLabel* lblProtocol = new QLabel("protocol", this);
	m_pleHost = new QLineEdit("test.mosquitto.org", this);
	m_psbPort = new QSpinBox(this);
	m_pleTopicName = new QLineEdit(this);
	m_pchbSecure = new QCheckBox("Secure", this);
	//m_pchbWebSockets = new QCheckBox("WebSockets", this);
	m_pcbProtocol = new QComboBox(this);
	m_pbtnConnect = new QPushButton("Connect", this);
	m_ppteLogMessages = new QPlainTextEdit(this);
	//unsubscribe combobox
	m_pbtnTopicSubscribe = new QPushButton("Subscribe", this);
	QGroupBox* groupConnectionSettings = new QGroupBox("Connection settings", this);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	QGridLayout* connectionSettingsLayout = new QGridLayout(groupConnectionSettings);
	QHBoxLayout* tempLayout = new QHBoxLayout; //todo delete this

	connectionSettingsLayout->addWidget(lblHost, 0, 0);
	connectionSettingsLayout->addWidget(m_pleHost, 0, 1);
	connectionSettingsLayout->addWidget(lblPort, 1, 0);
	connectionSettingsLayout->addWidget(m_psbPort, 1, 1);
	connectionSettingsLayout->addWidget(m_pchbSecure, 2, 0);
	connectionSettingsLayout->addWidget(lblProtocol, 3, 0);
	connectionSettingsLayout->addWidget(m_pcbProtocol, 3, 1);

	tempLayout->addWidget(lblTopicName);
	tempLayout->addWidget(m_pleTopicName);
	tempLayout->addWidget(m_pbtnTopicSubscribe);

	mainLayout->addWidget(groupConnectionSettings);
	mainLayout->addWidget(m_pbtnConnect);
	mainLayout->addLayout(tempLayout);
	mainLayout->addStretch(1);
	mainLayout->addWidget(lblLogMessages, Qt::AlignLeft);
	mainLayout->addWidget(m_ppteLogMessages);
	
	connect(m_pmqttClient, &QMqttClient::disconnected, this, &MQTTConnectionManager::onDisconnectBroker);
	connect(m_pmqttClient, &QMqttClient::messageReceived, this, &MQTTConnectionManager::onMessageReceived);
	connect(m_psbPort, &QSpinBox::valueChanged, this, &MQTTConnectionManager::setClientPort);
	connect(m_pbtnConnect, &QPushButton::clicked, this, &MQTTConnectionManager::onBtnConnectClick);
	connect(m_pbtnTopicSubscribe, &QPushButton::clicked, this, &MQTTConnectionManager::onBtnSubscribeClick);
	connect(m_pchbSecure, &QCheckBox::checkStateChanged, this, [this] { m_bSecure = m_pchbSecure->isChecked(); });
	//connect(m_pchbWebSockets, &QCheckBox::checkStateChanged, this, [this] {m_bWebSockets = m_pchbWebSockets->isChecked(); });
	connect(m_pcbProtocol, &QComboBox::currentIndexChanged, this, [this] (int ind) {
		m_protocolVersion = static_cast<QMqttClient::ProtocolVersion>(ind + 3);
	});

	m_ppteLogMessages->setReadOnly(true);
	m_pcbProtocol->addItems({
		"MQTT_3_1",
		"MQTT_3_1_1",
		"MQTT_5_0"
		});
	m_psbPort->setMaximum(65535); //numeric_limit<quint16>
	m_psbPort->setValue(1883);

	resize(500, 400);
}

void MQTTConnectionManager::onBtnConnectClick() {
	m_pmqttClient->setHostname(m_pleHost->text());
	m_pmqttClient->setPort(m_uiPort);

	if (m_pmqttClient->state() == QMqttClient::Disconnected) {
		m_pleHost->setEnabled(false);
		m_psbPort->setEnabled(false);
		m_pchbSecure->setEnabled(false);
		m_pcbProtocol->setEnabled(false);

		m_pmqttClient->setProtocolVersion(m_protocolVersion);

		m_pbtnConnect->setText("Disconnect");

		//update to 6.10 qt
//		if (m_bSecure && m_bWebSockets)
//			m_pmqttClient->connectToH
#if !defined(QT_NO_SSL)
		if (m_bSecure)
			m_pmqttClient->connectToHostEncrypted({});
#endif
		if (!m_bSecure)
			m_pmqttClient->connectToHost();

		if (m_pmqttClient->state() == QMqttClient::Disconnected) {
			m_pleHost->setEnabled(true);
			m_psbPort->setEnabled(true);
			m_pchbSecure->setEnabled(true);
			m_pcbProtocol->setEnabled(true);

			m_pbtnConnect->setText("Connect");
			m_pmqttClient->disconnectFromHost();
		}
	}
	else {
		m_pleHost->setEnabled(true);
		m_psbPort->setEnabled(true);
		m_pchbSecure->setEnabled(true);
		m_pcbProtocol->setEnabled(true);

		m_pbtnConnect->setText("Connect");
		m_pmqttClient->disconnectFromHost();
	}
}

void MQTTConnectionManager::onBtnSubscribeClick() {
	using Qt::StringLiterals::operator ""_s;

	auto subscription = m_pmqttClient->subscribe(m_pleTopicName->text());
	if (!subscription) 
		QMessageBox::critical(this, u"Error"_s, u"Could not a subscribe"_s);
}

void MQTTConnectionManager::onMessageReceived(const QByteArray& message, const QMqttTopicName& topic) {
	m_ppteLogMessages->insertPlainText(QDateTime::currentDateTime().toString() +
		" Received topic:" + topic.name() +
		" Message: " + message +
		"\n"
	);
}

void MQTTConnectionManager::onDisconnectBroker() {
	m_pleHost->setEnabled(true);
	m_psbPort->setEnabled(true);
	m_pchbSecure->setEnabled(true);
	m_pcbProtocol->setEnabled(true);

	m_pbtnConnect->setText("Connect");
}

void MQTTConnectionManager::setClientPort(int port) {
	m_uiPort = static_cast<quint16>(port);
}
