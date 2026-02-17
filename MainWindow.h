#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QWidget>
#include <QStandardItemModel>
#include <QMap>

#include "TreeModel.h"
#include "MQTTConnectionManager.h"


class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
private:
	void addCentralWidget();
	void addDockTreeTopicsNames();

	MQTTConnectionManager* m_pMqttConnectionManager = nullptr;

	QTreeView* m_pTreeWidgetTopics = nullptr;
	TreeModel* m_pModel = nullptr;
	QList<QTreeWidgetItem*> lstTreeItems;

	QMap<QString, QModelIndex> m_mapPathIndex;
	QMap<QModelIndex, QString> m_mapIndexPath;
signals:
	void clickedTopic(const QString& topicName);

public slots:
	void onSubscriptionMessageReceived(QMqttMessage);
};

#endif //MAIN_WINDOW_H
