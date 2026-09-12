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
	// Monotonic frame counter, exposed through getFrameNumber() so an automated driver can
	// wait a definite number of frames rather than sleeping and hoping.
	uint64_t frameNumber = 0;

	up<FrameworkPrivate> p;
	UString programName;
	bool createWindow;
	void audioInitialise(bool headless);
	void audioShutdown();

	static Framework *instance;

	up<ApocCursor> cursor;

	std::list<StageCmd> stageCommands;

	UString language;
	UString languageCountry;

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
	// Resize the window as if the user dragged it. Automation hook: the harness RESIZE command
	// needs a deterministic display size, since a driver reasoning about screen coordinates
	// cannot depend on whatever size the window happened to open at.
	void displaySetSize(Vec2<int> size);
	// Read the last rendered frame back and write it to disk. Automation hook for SCREENSHOT.
	bool writeScreenshot(const UString &path);
	void displaySetTitle(UString NewTitle);
	void displaySetIcon(sp<RGBImage> icon);
	bool displayHasWindow() const;
	void *getWindowHandle() const;

	// Map coordinates from window to display, for scaled displays
	int coordWindowToDisplayX(int x) const;
	int coordWindowToDisplayY(int y) const;
	Vec2<int> coordWindowsToDisplay(const Vec2<int> &coord) const;

	// Frame counter, so an automated driver can wait a definite number of frames rather than
	// sleeping and hoping.
	uint64_t getFrameNumber() const { return frameNumber; }

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

	void setMouseGrab();

	UString textGetClipboard();

	void threadPoolTaskEnqueue(std::function<void()> task);
	// add new work item to the pool
	template <class F, class... Args>
	auto threadPoolEnqueue(F &&f, Args &&...args)
	    -> std::shared_future<typename std::invoke_result_t<F, Args...>>
	{
		using return_type = typename std::invoke_result_t<F, Args...>;

		auto task = std::make_shared<std::packaged_task<return_type()>>(
		    std::bind(std::forward<F>(f), std::forward<Args>(args)...));

		std::shared_future<return_type> res = task->get_future().share();
		this->threadPoolTaskEnqueue(
		    [task, res]()
		    {
			    (*task)();
			    // Without a future.get() any exceptions are dropped on the floor
			    res.get();
		    });
		return res;
	}

	UString getDataDir() const;
	UString getCDPath() const;

	void setupModDataPaths();
	const UString &getLanguage() const { return this->language; };
	const UString &getLanguageCountry() const { return this->languageCountry; };
};

static inline Framework &fw() { return Framework::getInstance(); }

}; // namespace OpenApoc
