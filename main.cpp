#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);
	//сканим первый уровень /остальные по открытию в дереве
	MainWindow* mainWindow = new MainWindow;
	mainWindow->show();

	app.exec();
	return 0;
}
