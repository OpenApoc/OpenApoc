#include "launcherwindow.h"
#include <QApplication>
#include <SDL_main.h>

int main(int argc, char **argv)
{
	QApplication LauncherApp(argc, argv);
	LauncherWindow Launcher;
	Launcher.show();

	return LauncherApp.exec();
}
