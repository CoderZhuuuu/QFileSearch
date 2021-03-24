#include "QFileSearch.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QFileSearch w;
	w.show();
	a.installNativeEventFilter(w.filter);
	return a.exec();
}
