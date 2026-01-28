#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QWidget>
#include <QStandardItemModel>

#include <unordered_map>

#include "TreeModel.h"
#include "MQTTConnectionManager.h"


class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
private:
	void addDockTreeTopicsNames();

	MQTTConnectionManager* m_pMqttConnectionManager = nullptr;

	QTreeView* m_pTreeWidgetTopics = nullptr;
	TreeModel* m_pModel = nullptr;
	QList<QTreeWidgetItem*> lstTreeItems;

	std::unordered_map<QString, QModelIndex> m_mapTopicNameIndex;
public slots:
	void onMessageReceived(const QByteArray& message, const QMqttTopicName& topic);
};

#endif //MAIN_WINDOW_H
