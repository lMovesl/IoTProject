#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QWidget>

#include "MQTTConnectionManager.h"

class MainWindow : public QMainWindow {
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
private:
	void addDockTreeTopicsNames();

	MQTTConnectionManager* m_pMqttConnectionManager = nullptr;
};

#endif //MAIN_WINDOW_H
