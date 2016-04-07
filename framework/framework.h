#pragma once

#include "framework/ThreadPool/ThreadPool.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/includes.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/stagestack.h"
#include "library/sp.h"
#include "library/strings.h"

// FIXME: Remove SDL headers - we currently use SDL types directly in input events
#include <SDL_keyboard.h>
#include <SDL_keycode.h>
#include <SDL_mouse.h>
#include <SDL_scancode.h>

namespace OpenApoc
{

class Shader;
class GameCore;
class FrameworkPrivate;
class ApocCursor;

#define FRAMES_PER_SECOND 100

class Framework
{
  private:
	std::unique_ptr<FrameworkPrivate> p;
	UString programName;
	bool createWindow;
	void Audio_Initialise();
	void Audio_Shutdown();

	static Framework *instance;

	up<ApocCursor> cursor;

  public:
	std::unique_ptr<Data> data;
	std::unique_ptr<ConfigFile> Settings;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<SoundBackend> soundBackend;
	std::unique_ptr<JukeBox> jukebox;

	std::unique_ptr<ThreadPool> threadPool;

	Framework(const UString programName, const std::vector<UString> cmdline,
	          bool createWindow = true);
	~Framework();

	static Framework &getInstance();

	void Run(sp<Stage> initialStage);
	void ProcessEvents();
	void PushEvent(Event *e);
	void TranslateSDLEvents();
	void ShutdownFramework();
	bool IsShuttingDown();

	void SaveSettings();

	void Display_Initialise();
	void Display_Shutdown();
	int Display_GetWidth();
	int Display_GetHeight();
	Vec2<int> Display_GetSize();
	void Display_SetTitle(UString NewTitle);
	void Display_SetIcon();

	bool IsSlowMode();
	void SetSlowMode(bool SlowEnabled);

	sp<Stage> Stage_GetPrevious();
	sp<Stage> Stage_GetPrevious(sp<Stage> From);

	Vec2<int> getCursorPosition();
};

static inline Framework &fw() { return Framework::getInstance(); }

}; // namespace OpenApoc
