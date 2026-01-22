#include "MainWindow.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <qlayout.h>
#include <QTreeView>
#include <QHeaderView>

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	m_pMqttConnectionManager = new MQTTConnectionManager;

	QMenu* mainMenu = new QMenu("&Настройки", this);
	QAction* actionConnectionSettings = new QAction("Настройки подключения" ,mainMenu);

	mainMenu->addAction(actionConnectionSettings);
	menuBar()->addMenu(mainMenu);

	connect(actionConnectionSettings, &QAction::triggered, m_pMqttConnectionManager, &MQTTConnectionManager::show);

	addDockTreeTopicsNames();

	setWindowState(Qt::WindowMaximized);
}

void MainWindow::addDockTreeTopicsNames() {
	QDockWidget* dockWidgetTopics = new QDockWidget("Topics", this);
	m_pTreeWidgetTopics = new QTreeView(dockWidgetTopics);
	
	m_pTreeWidgetTopics->setModel(m_pModel);

	m_pTreeWidgetTopics->header()->hide();
	dockWidgetTopics->setWidget(m_pTreeWidgetTopics);

	connect(m_pMqttConnectionManager->getMqttClient(), &QMqttClient::messageReceived, 
		this, &MainWindow::onMessageReceived);
	
	addDockWidget(Qt::LeftDockWidgetArea, dockWidgetTopics);
}

void MainWindow::onMessageReceived(const QByteArray& message, const QMqttTopicName& topic) {
	QStandardItem* topicTreeItem = new QStandardItem(topic.name());
}
