#include "MainWindow.h"
#include <utility>

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
	m_pModel = new TreeModel({ "TopicName" });

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
	QString topicName = topic.name();
	if (m_mapTopicNameIndex.contains(topicName))
		return;

	QString path = "";
	QModelIndex parent;
	for (const QString& level : topic.levels())
	{
		path += level + '/';
		if (!m_mapTopicNameIndex.contains(path))
		{
			int row = m_pModel->rowCount(parent);

			m_pModel->insertRows(row, 1, parent);
			auto newItemIndex = m_pModel->index(row, 0, parent);
			m_pModel->setData(newItemIndex, level);

			parent = newItemIndex;
			m_mapTopicNameIndex[path] = newItemIndex;
		}
		else
			parent = m_mapTopicNameIndex[path];
	}
}
