#include <QApplication>

#include "MQTTConnectionManager.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	//сканим первый уровень /остальные по открытию в дереве
	MQTTConnectionManager* connManager = new MQTTConnectionManager;
	connManager->show();

	app.exec();
	return 0;
}
