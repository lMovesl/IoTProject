#include "MainWindow.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <qlayout.h>
#include <QTreeWidget>
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
	QTreeWidget* treeWidgetTopics = new QTreeWidget(dockWidgetTopics);

	treeWidgetTopics->header()->hide();
	dockWidgetTopics->setWidget(treeWidgetTopics);

	addDockWidget(Qt::LeftDockWidgetArea, dockWidgetTopics);
}
