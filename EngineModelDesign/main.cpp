#include "EngineModelDesign.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	EngineModelDesign w;
	w.show();
	return a.exec();
}
