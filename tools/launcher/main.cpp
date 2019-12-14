// WinMain is already defined in qt, so we don't want to accidently include the
// SDL one
#define SDL_MAIN_HANDLED
#include <SDL.h>

#include "launcherwindow.h"
#include <QApplication>
int main(int argc, char **argv)
{
	SDL_SetMainReady();
	QApplication LauncherApp(argc, argv);
	LauncherWindow Launcher;
	Launcher.show();

	return LauncherApp.exec();
}
