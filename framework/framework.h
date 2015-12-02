
#pragma once
#include "library/sp.h"

#include "logger.h"
#include "includes.h"
#include "event.h"
#include "data.h"
#include "stagestack.h"
#include "renderer.h"
#include "sound.h"
#include "font.h"

#include "framework/ThreadPool/ThreadPool.h"

#include "library/configfile.h"
#include "library/strings.h"

#include "game/gamestate.h"
#include "game/resources/gamecore.h"
#include "game/rules/rules.h"

// FIXME: Remove core-allegro
// Required for input types
#include <allegro5/allegro.h>

namespace OpenApoc
{

class Shader;
class GameCore;

#define FRAMES_PER_SECOND 100

class FrameworkPrivate;

class Framework
{
  private:
	std::unique_ptr<FrameworkPrivate> p;
	UString programName;
	void Audio_Initialise();
	void Audio_Shutdown();

  public:
	std::unique_ptr<Data> data;
	std::unique_ptr<GameState> state;
	std::unique_ptr<Rules> rules;
	std::unique_ptr<GameCore> gamecore;

	std::unique_ptr<ConfigFile> Settings;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<SoundBackend> soundBackend;
	std::unique_ptr<JukeBox> jukebox;

	std::unique_ptr<ThreadPool> threadPool;

	Framework(const UString programName, const std::vector<UString> cmdline);
	~Framework();

	void Run();
	void ProcessEvents();
	void PushEvent(Event *e);
	void TranslateAllegroEvents();
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
};

}; // namespace OpenApoc
