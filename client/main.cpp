#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
	std::setlocale(LC_ALL, "RU");
	
	QApplication app(argc, argv);
	MainWindow* mainWindow = new MainWindow();
	mainWindow->showMaximized();
	app.exec();
	return 0;
}
