#include "MainWindow.h"

#include <utility>

#include <QLabel>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDockWidget>
#include <QLayout>
#include <QTreeView>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MainWindow::MainWindow(QWidget* parent, Qt::WindowFlags flags) : QMainWindow(parent, flags) {
	m_pMqttConnectionManager = new MQTTConnectionManager;

	QMenu* mainMenu = new QMenu("&Настройки", this);
	QAction* actionConnectionSettings = new QAction("Настройки подключения" ,mainMenu);

	mainMenu->addAction(actionConnectionSettings);
	menuBar()->addMenu(mainMenu);

	connect(actionConnectionSettings, &QAction::triggered, m_pMqttConnectionManager, &MQTTConnectionManager::show);

	addCentralWidget();
	addDockTreeTopicsNames();

	setWindowState(Qt::WindowMaximized);
}

void MainWindow::addCentralWidget() {
	QWidget* centralWidget = new QWidget(this);
	QLabel* lblTopicName = new QLabel(centralWidget);
	QTextEdit* pteMessage = new QTextEdit(centralWidget);

	QVBoxLayout* pvbxMainLayout = new QVBoxLayout(centralWidget);

	pvbxMainLayout->addWidget(lblTopicName);
	pvbxMainLayout->addWidget(pteMessage);

	centralWidget->setLayout(pvbxMainLayout);

	this->setCentralWidget(centralWidget);

	connect(this, &MainWindow::clickedTopic, [lblTopicName, pteMessage]()
	{
		lblTopicName->clear();
		pteMessage->clear();
	});
	connect(this, &MainWindow::clickedTopic, lblTopicName, &QLabel::setText);
	connect(m_pMqttConnectionManager, &MQTTConnectionManager::subscriptionMessageReceive,
		[this, lblTopicName, pteMessage](QMqttMessage message)
		{
			if (lblTopicName->text().isEmpty())
				return;
			QJsonParseError jsonParseError;
			QJsonDocument jsonDoc = QJsonDocument::fromJson(message.payload(), &jsonParseError);
			QString topicName = message.topic().name();
			//qDebug() <<"topic" <<lblTopicName->text();
			//qDebug() <<"mess" <<topicName;
			if (lblTopicName->text() == topicName)
				pteMessage->setText(jsonDoc.toJson(QJsonDocument::JsonFormat::Indented));
		});
}

void MainWindow::addDockTreeTopicsNames() {
	m_pModel = new TreeModel({ "TopicName" });

	QDockWidget* dockWidgetTopics = new QDockWidget("Topics", this);

	m_pTreeWidgetTopics = new QTreeView(dockWidgetTopics);
	m_pTreeWidgetTopics->setModel(m_pModel);
	m_pTreeWidgetTopics->header()->hide();

	dockWidgetTopics->setWidget(m_pTreeWidgetTopics);

	connect(m_pTreeWidgetTopics, &QTreeView::clicked, [this](const QModelIndex& index)
	{
		QString topicName = m_mapIndexPath[index];
		emit clickedTopic(topicName);
	});
	connect(m_pMqttConnectionManager, &MQTTConnectionManager::subscriptionMessageReceive,
		this, &MainWindow::onSubscriptionMessageReceived);

	addDockWidget(Qt::LeftDockWidgetArea, dockWidgetTopics);
}

void MainWindow::onSubscriptionMessageReceived(QMqttMessage message) {
	//пишем в бд
	//либо в сыром формате выводить либо рекурсию
	auto topic = message.topic();
	QString topicName = topic.name(); 
	
	
	
	//if (jsonDoc.isNull()) {
	//	//qDebug() << topicName << " " << jsonParseError.errorString();
	//}
	//else if (jsonDoc.isObject())
	//{
	//	QJsonObject jsonObject = jsonDoc.object();
	//	qDebug() << jsonObject;
	//}
	//else if (jsonDoc.isArray())
	//{
	//	QJsonArray jsonArray = jsonDoc.array();

	//	qDebug() << jsonArray;
	//	for (const auto& jsonVal : jsonArray)
	//	{
	//		//json key
	//		qDebug() << jsonVal.toString();
	//	}

	//}
	//else
	//{
	//	QVariant jsonValue = jsonDoc.toVariant();
	//}

	if (m_mapPathIndex.contains(topicName)) //topicname вместо qstring
		return;

	QStringList topicLevels;
	QModelIndex parent;
	for (const QString& level : topic.levels())
	{
		topicLevels.append(level);
		QString path = topicLevels.join('/');
		if (!m_mapPathIndex.contains(path))
		{
			int row = m_pModel->rowCount(parent);

			m_pModel->insertRows(row, 1, parent);
			auto newItemIndex = m_pModel->index(row, 0, parent);
			m_pModel->setData(newItemIndex, level);

			parent = newItemIndex;
			m_mapPathIndex[path] = newItemIndex;
			m_mapIndexPath[newItemIndex] = path;
		}
		else
			parent = m_mapPathIndex[path];
	}
}

