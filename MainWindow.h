#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QWidget>
#include <QStandardItemModel>

#include "MQTTConnectionManager.h"


class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
private:
	void addDockTreeTopicsNames();

	MQTTConnectionManager* m_pMqttConnectionManager = nullptr;

	QTreeView* m_pTreeWidgetTopics = nullptr;
	QStandardItemModel* m_pModel = nullptr;
	QList<QTreeWidgetItem*> lstTreeItems;
public slots:
	void onMessageReceived(const QByteArray& message, const QMqttTopicName& topic);
};

#endif //MAIN_WINDOW_H
