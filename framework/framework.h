#pragma once

#include "framework/modinfo.h"
#include "library/sp.h"
#include "library/strings.h"
#include "library/vec.h"
#include <functional>
#include <future>

namespace OpenApoc
{

class Shader;
class GameCore;
class FrameworkPrivate;
class ApocCursor;
class Event;
class Image;
class Data;
class Renderer;
class SoundBackend;
class JukeBox;
class StageCmd;
class Stage;
class RGBImage;

#define FRAMES_PER_SECOND 100

class Framework
{
  private:
	up<FrameworkPrivate> p;
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

	Framework(const UString programName, bool createWindow = true);
	~Framework();

	static Framework &getInstance();
	static Framework *tryGetInstance();

	void run(sp<Stage> initialStage);
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
	void displaySetIcon(sp<RGBImage> icon);
	bool displayHasWindow() const;
	void *getWindowHandle() const;

	bool isSlowMode();
	void setSlowMode(bool SlowEnabled);

	sp<Stage> stageGetCurrent();
	sp<Stage> stageGetPrevious();
	sp<Stage> stageGetPrevious(sp<Stage> From);

	void stageQueueCommand(const StageCmd &cmd);

	ApocCursor &getCursor();

	void textStartInput();
	void textStopInput();

	void toolTipStartTimer(up<Event> e);
	void toolTipStopTimer();
	void toolTipTimerCallback(unsigned int interval, void *data);
	void showToolTip(sp<Image> image, const Vec2<int> &position);

	UString textGetClipboard();

	void threadPoolTaskEnqueue(std::function<void()> task);
	// add new work item to the pool
	template <class F, class... Args>
	auto threadPoolEnqueue(F &&f, Args &&... args)
	    -> std::shared_future<typename std::result_of<F(Args...)>::type>
	{
		using return_type = typename std::result_of<F(Args...)>::type;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
		    std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::shared_future<return_type> res = task->get_future().share();
		this->threadPoolTaskEnqueue([task, res]() {
			(*task)();
			// Without a future.get() any exceptions are dropped on the floor
			res.get();
		});
		return res;
	}

	UString getDataDir() const;
	UString getCDPath() const;

	void setupModDataPaths();
};

static inline Framework &fw() { return Framework::getInstance(); }

}; // namespace OpenApoc
