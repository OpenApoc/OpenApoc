#pragma once

#include "framework/ThreadPool/ThreadPool.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/renderer.h"
#include "framework/sound.h"
#include "framework/stagestack.h"
#include "library/sp.h"
#include "library/strings.h"

namespace OpenApoc
{

class Shader;
class GameCore;
class FrameworkPrivate;
class ApocCursor;
class Event;

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

	// If frameCount != 0, it'll quit after that many frames. If it is zero, it'll run forever (Or
	// until a user quit event)
	void Run(sp<Stage> initialStage, size_t frameCount = 0);
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

	void Text_StartInput();
	void Text_StopInput();
};

static inline Framework &fw() { return Framework::getInstance(); }

}; // namespace OpenApoc
