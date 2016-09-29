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
	void audioInitialise();
	void audioShutdown();

	static Framework *instance;

	up<ApocCursor> cursor;

	std::list<StageCmd> stageCommands;

  public:
	std::unique_ptr<Data> data;
	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<SoundBackend> soundBackend;
	std::unique_ptr<JukeBox> jukebox;

	std::unique_ptr<ThreadPool> threadPool;

	Framework(const UString programName, bool createWindow = true);
	~Framework();

	static Framework &getInstance();
	static Framework *tryGetInstance();

	// If frameCount != 0, it'll quit after that many frames. If it is zero, it'll run forever (Or
	// until a user quit event)
	void run(sp<Stage> initialStage, size_t frameCount = 0);
	void processEvents();
	/* PushEvent() take ownership of the Event, and will delete it after use*/
	void pushEvent(up<Event> e);
	void pushEvent(Event *e);

	void translateSdlEvents();
	void shutdownFramework();
	bool isShuttingDown();

	void displayInitialise();
	void displayShutdown();
	int displayGetWidth();
	int displayGetHeight();
	Vec2<int> displayGetSize();
	void displaySetTitle(UString NewTitle);
	void displaySetIcon();
	bool displayHasWindow() const;

	bool isSlowMode();
	void setSlowMode(bool SlowEnabled);

	sp<Stage> stageGetCurrent();
	sp<Stage> stageGetPrevious();
	sp<Stage> stageGetPrevious(sp<Stage> From);

	void stageQueueCommand(const StageCmd &cmd);

	ApocCursor &getCursor();

	void textStartInput();
	void textStopInput();
	UString textGetClipboard();
};

static inline Framework &fw() { return Framework::getInstance(); }

}; // namespace OpenApoc
