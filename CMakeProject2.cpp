
#include <QApplication>

#include "CMakeProject2.h"
#include "MQTTConnectionManager.h"

using namespace std;

int main(int argc, char* argv[]) {
	QApplication app(argc, argv);

	MQTTConnectionManager connManager;
	connManager.show();

	app.exec();
	return 0;
}
