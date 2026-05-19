#include "MQTTConnectionManager.h"

#include <QLabel>
#include <QGridLayout>
#include <QDateTime>
#include <QMessageBox>
#include <QGroupBox>
#include <QMetaEnum>
#include <QSettings>

MQTTConnectionManager::MQTTConnectionManager(QMqttClient* client, QWidget* parent) : QDialog(parent) {
	m_pmqttClient = client;
	QLabel* lblHost = new QLabel("Host", this);
	QLabel* lblPort = new QLabel("Port", this);
	QLabel* lblLogMessages = new QLabel("Log messages", this);
	QLabel* lblProtocol = new QLabel("Protocol", this);
	QLabel* lblState = new QLabel("Connection state:", this);
	QLabel* lblCurrentState = new QLabel("", this);
	m_pleHost = new QLineEdit("test.mosquitto.org", this);
	m_psbPort = new QSpinBox(this);
	m_pcbProtocol = new QComboBox(this);
	m_pbtnConnect = new QPushButton("Connect", this);
	m_ppteLogMessages = new QPlainTextEdit(this);
	QGroupBox* groupConnectionSettings = new QGroupBox("Connection settings", this);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	QGridLayout* connectionSettingsLayout = new QGridLayout(groupConnectionSettings);
	QHBoxLayout* stateLayout = new QHBoxLayout;

	connectionSettingsLayout->addWidget(lblHost, 0, 0);
	connectionSettingsLayout->addWidget(m_pleHost, 0, 1);
	connectionSettingsLayout->addWidget(lblPort, 1, 0);
	connectionSettingsLayout->addWidget(m_psbPort, 1, 1);
	connectionSettingsLayout->addWidget(lblProtocol, 3, 0);
	connectionSettingsLayout->addWidget(m_pcbProtocol, 3, 1);

	stateLayout->addWidget(lblState);
	stateLayout->addWidget(lblCurrentState, Qt::AlignLeft);

	mainLayout->addWidget(groupConnectionSettings);
	mainLayout->addLayout(stateLayout);
	mainLayout->addWidget(m_pbtnConnect);
	mainLayout->addStretch(1);
	mainLayout->addWidget(lblLogMessages, Qt::AlignLeft);
	mainLayout->addWidget(m_ppteLogMessages);
	
	connect(m_pmqttClient, &QMqttClient::disconnected, this, &MQTTConnectionManager::updateUiStates);
	connect(m_psbPort, &QSpinBox::valueChanged, this, &MQTTConnectionManager::setClientPort);
	connect(m_pbtnConnect, &QPushButton::clicked, this, &MQTTConnectionManager::onBtnConnectClick);
	connect(m_pcbProtocol, &QComboBox::currentIndexChanged, this, [this] (int ind) {
		m_protocolVersion = static_cast<QMqttClient::ProtocolVersion>(ind + 3);
	});
	connect(m_pmqttClient, &QMqttClient::errorChanged, this, [this](QMqttClient::ClientError error) {
			m_ppteLogMessages->appendPlainText("MQTT error: " + QString::number(error));
	});
	connect(m_pmqttClient, &QMqttClient::stateChanged, this, [this, lblCurrentState] (QMqttClient::ClientState state) {
		lblCurrentState->setText(QMetaEnum::fromType<QMqttClient::ClientState>().valueToKey(state));
		updateUiStates();
		});

	m_ppteLogMessages->setReadOnly(true);
	m_pcbProtocol->addItems({
		"MQTT_3_1",
		"MQTT_3_1_1",
		"MQTT_5_0"
		});
	m_psbPort->setMaximum(65535); //numeric_limit<quint16>
	m_psbPort->setValue(1883);

	m_pmqttClient->emit stateChanged(m_pmqttClient->state());

	resize(500, 400);

	loadSettingsFromClient();
}

QMqttClient* MQTTConnectionManager::getMqttClient() const {
	return m_pmqttClient;
}

void MQTTConnectionManager::onBtnConnectClick() {
	if (!m_pmqttClient) return;

	if (m_pmqttClient->state() == QMqttClient::Disconnected) {
		m_pmqttClient->setHostname(m_pleHost->text().trimmed());
		m_pmqttClient->setPort(static_cast<quint16>(m_psbPort->value()));

		saveSettings();

		m_pbtnConnect->setText("Disconnect"); 
		m_pmqttClient->connectToHost();
	}
	else {
		m_pmqttClient->disconnectFromHost();
		m_pbtnConnect->setText("Connect");
	}
}

void MQTTConnectionManager::setClientPort(int port) {
	m_uiPort = static_cast<quint16>(port);
}

void MQTTConnectionManager::saveSettings() {
	QSettings settings("YourOrganization", "IoTMonitoringSystem");
	settings.beginGroup("MQTT");
	settings.setValue("host", m_pleHost->text().trimmed());
	settings.setValue("port", m_psbPort->value());
	settings.setValue("protocol", m_pcbProtocol->currentIndex());
	settings.endGroup();
}

void MQTTConnectionManager::loadSettingsFromClient() {
	if (!m_pmqttClient) return;

	m_pleHost->setText(m_pmqttClient->hostname());
	m_psbPort->setValue(m_pmqttClient->port());

	if (m_pmqttClient->protocolVersion() == QMqttClient::MQTT_3_1) {
		m_pcbProtocol->setCurrentIndex(0); 
	}
	else if (m_pmqttClient->protocolVersion() == QMqttClient::MQTT_3_1_1) {
		m_pcbProtocol->setCurrentIndex(1);
	}
	else if (m_pmqttClient->protocolVersion() == QMqttClient::MQTT_5_0) {
		m_pcbProtocol->setCurrentIndex(2);
	}
	QString clientId = m_pmqttClient->clientId();
	QString username = m_pmqttClient->username();

	m_pmqttClient->emit stateChanged(m_pmqttClient->state());
}

void MQTTConnectionManager::updateUiStates() {
	if (!m_pmqttClient) return;

	bool isConnected = (m_pmqttClient->state() == QMqttClient::Connected ||
		m_pmqttClient->state() == QMqttClient::Connecting);

	m_pleHost->setDisabled(isConnected);
	m_psbPort->setDisabled(isConnected);
	m_pcbProtocol->setDisabled(isConnected);

	if (m_pmqttClient->state() == QMqttClient::Connected) {
		m_pbtnConnect->setText("Disconnect");
	}
	else if (m_pmqttClient->state() == QMqttClient::Connecting) {
		m_pbtnConnect->setText(tr("Connecting..."));
	}
	else {
		m_pbtnConnect->setText(tr("Connect"));
	}
}