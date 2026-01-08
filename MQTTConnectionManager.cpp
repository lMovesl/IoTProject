#include "MQTTConnectionManager.h"

#include <QLabel>
#include <QGridLayout>

MQTTConnectionManager::MQTTConnectionManager(QWidget* parent) : QWidget(parent) {
	m_pmqttClient = new QMqttClient(this);

	QLabel* lblHost = new QLabel("Host", this);
	QLabel* lblPort = new QLabel("Port", this);
	m_pleHost = new QLineEdit("test.mosquitto.org",this);
	m_plePort = new QLineEdit("1883",this);
	m_pbtnConnect = new QPushButton("Connect", this);

	QGridLayout* mainLayout = new QGridLayout(this);

	mainLayout->addWidget(lblHost, 0, 0, Qt::AlignRight);
	mainLayout->addWidget(m_pleHost, 0, 1);
	mainLayout->addWidget(lblPort, 1, 0, Qt::AlignRight);
	mainLayout->addWidget(m_plePort, 1, 1);
	mainLayout->addWidget(m_pbtnConnect, 2, 0, 1, 2);

	connect(m_pbtnConnect, &QPushButton::clicked, this, &MQTTConnectionManager::onBtnConnectClick);
}

void MQTTConnectionManager::onBtnConnectClick() {
	m_pmqttClient->setHostname(m_pleHost->text());
	m_pmqttClient->setPort(static_cast<qint16>(m_plePort->text().toInt()));

	if (m_pmqttClient->state() == QMqttClient::Disconnected) {
		m_pleHost->setEnabled(false);
		m_plePort->setEnabled(false);

		m_pbtnConnect->setText("Disconnect");
		m_pmqttClient->connectToHost();

		if (m_pmqttClient->state() == QMqttClient::Disconnected) {
			m_pleHost->setEnabled(true);
			m_plePort->setEnabled(true);

			m_pbtnConnect->setText("Connect");
			m_pmqttClient->disconnectFromHost();
		}
	}
	else {
		m_pleHost->setEnabled(true);
		m_plePort->setEnabled(true);

		m_pbtnConnect->setText("Connect");
		m_pmqttClient->disconnectFromHost();
	}
}
