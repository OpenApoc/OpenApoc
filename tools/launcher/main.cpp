#include "launcherwindow.h"
#include <QApplication>

int main(int argc, char **argv)
{
	QApplication LauncherApp(argc, argv);
	LauncherWindow Launcher;
	Launcher.show();

	return LauncherApp.exec();
}
