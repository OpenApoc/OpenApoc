#include "framework/framework.h"
#include "framework/ThreadPool/ThreadPool.h"
#include "framework/apocresources/cursor.h"
#include "framework/configfile.h"
#include "framework/data.h"
#include "framework/event.h"
#include "framework/filesystem.h"
#include "framework/font.h"
#include "framework/image.h"
#include "framework/logger_file.h"
#include "framework/logger_sdldialog.h"
#include "framework/options.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include "framework/sound_interface.h"
#include "framework/stagestack.h"
#include "framework/trace.h"
#include "library/sp.h"
#include "library/xorshift.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <list>
#include <map>
#include <vector>

#ifdef __APPLE__
// Used for NASTY chdir() app bundle hacks
#include <unistd.h>
#endif

// SDL_syswm includes windows.h on windows, which does all kinds of polluting
// defines/namespace stuff, so try to avoid that
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL_syswm.h>

// Windows isn't the only thing that pollutes stuff with #defines - X gets in on it too with 'None'
#undef None

// Use physfs to get prefs dir
#include <physfs.h>

// Boost locale for setting the system locale
#include <boost/locale.hpp>

using namespace OpenApoc;

namespace OpenApoc
{

UString Framework::getDataDir() const { return Options::dataPathOption.get(); }

UString Framework::getCDPath() const { return Options::cdPathOption.get(); }

Framework *Framework::instance = nullptr;

// TODO: Make this moddable
const std::vector<std::vector<UString>> playlists = {
    // None
    {},
    // Cityscape Ambient
    {"music:0", "music:1", "music:2", "music:3", "music:4", "music:5", "music:6", "music:7",
     "music:8", "music:9"},
    // Tactical Ambient (also Cityscape Action)
    {"music:10", "music:11", "music:12", "music:13", "music:14", "music:15", "music:16", "music:17",
     "music:18", "music:19"},
    // Tactical Action
    {"music:20", "music:21", "music:22", "music:23", "music:24", "music:25", "music:26",
     "music:27"},
    // Alien Dimension
    {"music:28", "music:29", "music:30", "music:31", "music:32"}};

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<sp<MusicTrack>> trackList;
	PlayMode mode;
	PlayList list;
	Xorshift128Plus<uint64_t> rng;

  public:
	JukeBoxImpl(Framework &fw) : fw(fw), position(0), mode(PlayMode::Shuffle), list(PlayList::None)
	{
		// Use the time to give a little initial randomness to the shuffle rng
		auto time_now = std::chrono::system_clock::now();
		uint64_t time_seconds =
		    std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch()).count();
		rng.seed(time_seconds);
	}
	~JukeBoxImpl() override { this->stop(); }

	void shuffle() { std::shuffle(trackList.begin(), trackList.end(), rng); }

	void play(PlayList list, PlayMode mode) override
	{
		if (this->list == list)
			return;
		this->list = list;
		if (this->list == PlayList::None)
		{
			this->stop();
		}
		else
		{
			this->play(playlists[(int)list], mode);
		}
	}

	void play(const std::vector<UString> &tracks, PlayMode mode) override
	{
		this->trackList.clear();
		this->position = 0;
		this->mode = mode;
		for (auto &track : tracks)
		{
			auto musicTrack = fw.data->loadMusic(track);
			if (!musicTrack)
				LogError("Failed to load music track \"%s\" - skipping", track);
			else
				this->trackList.push_back(musicTrack);
		}
		if (mode == PlayMode::Shuffle)
			shuffle();
		this->progressTrack(this);
		this->fw.soundBackend->playMusic(progressTrack, this);
	}

	static void progressTrack(void *data)
	{
		JukeBoxImpl *jukebox = static_cast<JukeBoxImpl *>(data);
		if (jukebox->trackList.empty())
		{
			LogWarning("Trying to play empty jukebox");
			return;
		}
		if (jukebox->position >= jukebox->trackList.size())
		{
			LogInfo("End of jukebox playlist");
			return;
		}
		LogInfo("Playing track %u (%s)", jukebox->position,
		        jukebox->trackList[jukebox->position]->getName());
		jukebox->fw.soundBackend->setTrack(jukebox->trackList[jukebox->position]);

		jukebox->position++;
		if (jukebox->position >= jukebox->trackList.size())
		{
			if (jukebox->mode == PlayMode::Loop)
			{
				jukebox->position = 0;
			}
			else if (jukebox->mode == PlayMode::Shuffle)
			{
				jukebox->position = 0;
				jukebox->shuffle();
			}
		}
	}

	void stop() override
	{
		this->list = PlayList::None;
		fw.soundBackend->stopMusic();
	}
};

class FrameworkPrivate
{
  private:
	friend class Framework;
	bool quitProgram;

	SDL_DisplayMode screenMode;
	SDL_Window *window;
	SDL_GLContext context;

	std::map<UString, std::unique_ptr<RendererFactory>> registeredRenderers;
	std::map<UString, std::unique_ptr<SoundBackendFactory>> registeredSoundBackends;

	std::list<up<Event>> eventQueue;
	std::mutex eventQueueLock;

	StageStack ProgramStages;
	sp<Surface> defaultSurface;
	// The display size may be scaled up to windowSize
	Vec2<int> displaySize;
	Vec2<int> windowSize;

	sp<Surface> scaleSurface;
	up<ThreadPool> threadPool;

	int toolTipTimerId = 0;
	up<Event> toolTipTimerEvent;
	sp<Image> toolTipImage;
	Vec2<int> toolTipPosition;

	FrameworkPrivate()
	    : quitProgram(false), window(nullptr), context(0), displaySize(0, 0), windowSize(0, 0)
	{
		int threadPoolSize = Options::threadPoolSizeOption.get();
		if (threadPoolSize > 0)
		{
			LogInfo("Set thread pool size to %d", threadPoolSize);
		}
		else if (std::thread::hardware_concurrency() != 0)
		{
			threadPoolSize = std::thread::hardware_concurrency();
			LogInfo("Set thread pool size to reported HW concurrency of %d", threadPoolSize);
		}
		else
		{
			threadPoolSize = 2;
			LogInfo("Failed to get HW concurrency, falling back to pool size %d", threadPoolSize);
		}

		this->threadPool.reset(new ThreadPool(threadPoolSize));
	}
};

Framework::Framework(const UString programName, bool createWindow)
    : p(new FrameworkPrivate), programName(programName), createWindow(createWindow)
{
	TRACE_FN;
	LogInfo("Starting framework");

	if (this->instance)
	{
		LogError("Multiple Framework instances created");
	}

	this->instance = this;

#ifdef __APPLE__
	{
		// FIXME: A hack to set the working directory to the Resources directory in the app bundle.
		char *basePath = SDL_GetBasePath();
		// FIXME: How to check we're being run from the app bundle and not directly from the
		// terminal? On my testing (macos 10.15.1 19B88) it seems to have a "/" working directory,
		// which is unlikely in terminal use, so use that?
		if (fs::current_path() == "/")
		{
			LogWarning("Setting working directory to \"%s\"", basePath);
			chdir(basePath);
		}
		else
		{
			LogWarning("Leaving default working directory \"%s\"", fs::current_path());
		}
		SDL_free(basePath);
	}
#endif

	if (!PHYSFS_isInit())
	{
		if (PHYSFS_init(programName.cStr()) == 0)
		{
			PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
			LogError("Failed to init code %i PHYSFS: %s", (int)error, PHYSFS_getErrorByCode(error));
		}
	}
#ifdef ANDROID
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	// Initialize subsystems separately?
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER) < 0)
	{
		LogError("Cannot init SDL2");
		LogError("SDL error: %s", SDL_GetError());
		p->quitProgram = true;
		return;
	}
	if (createWindow)
	{
		if (SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		{
			LogError("Cannot init SDL_VIDEO - \"%s\"", SDL_GetError());
			p->quitProgram = true;
			return;
		}
	}
	LogInfo("Loading config\n");
	p->quitProgram = false;
	UString settingsPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	settingsPath += "/settings.cfg";

	UString logPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	std::ifstream portableFile("./portable.txt");
	if (portableFile)
	{
		logPath = ".";
	}

	logPath += "/log.txt";

	enableFileLogger(logPath.cStr());

	Options::dumpOptionsToLog();

	// This is always set, the default being an empty string (which correctly chooses 'system
	// language')
	UString desiredLanguageName;
	if (!Options::languageOption.get().empty())
	{
		desiredLanguageName = Options::languageOption.get();
	}

	LogInfo("Setting up locale \"%s\"", desiredLanguageName);

	boost::locale::generator gen;

	std::vector<UString> resourcePaths;
	resourcePaths.push_back(Options::cdPathOption.get());
	resourcePaths.push_back(Options::dataPathOption.get());

	for (auto &path : resourcePaths)
	{
		auto langPath = path + "/languages";
		LogInfo("Adding \"%s\" to language path", langPath);
		gen.add_messages_path(langPath.str());
	}

	std::vector<UString> translationDomains = {"paedia_string", "ufo_string"};
	for (auto &domain : translationDomains)
	{
		LogInfo("Adding \"%s\" to translation domains", domain);
		gen.add_messages_domain(domain.str());
	}

	std::locale loc = gen(desiredLanguageName.str());
	std::locale::global(loc);

	auto localeName = std::use_facet<boost::locale::info>(loc).name();
	auto localeLang = std::use_facet<boost::locale::info>(loc).language();
	auto localeCountry = std::use_facet<boost::locale::info>(loc).country();
	auto localeVariant = std::use_facet<boost::locale::info>(loc).variant();
	auto localeEncoding = std::use_facet<boost::locale::info>(loc).encoding();
	auto isUTF8 = std::use_facet<boost::locale::info>(loc).utf8();

	LogInfo("Locale info: Name \"%s\" language \"%s\" country \"%s\" variant \"%s\" encoding "
	        "\"%s\" utf8:%s",
	        localeName.c_str(), localeLang.c_str(), localeCountry.c_str(), localeVariant.c_str(),
	        localeEncoding.c_str(), isUTF8 ? "true" : "false");

	this->data.reset(Data::createData(resourcePaths));

	auto testFile = this->data->fs.open("music");
	if (!testFile)
	{
		LogError("Failed to open \"music\" from the CD - likely the cd couldn't be loaded or paths "
		         "are incorrect if using an extracted CD image");
	}

	auto testFile2 = this->data->fs.open("filedoesntexist");
	if (testFile2)
	{
		LogError("Succeeded in opening \"FileDoesntExist\" - either you have the weirdest filename "
		         "preferences or something is wrong");
	}
	srand(static_cast<unsigned int>(SDL_GetTicks()));

	if (createWindow)
	{
		displayInitialise();
		enableSDLDialogLogger(p->window);
		audioInitialise();
	}
}

Framework::~Framework()
{
	TRACE_FN;
	LogInfo("Destroying framework");
	// Stop any audio first, as if you've got ongoing music/samples it could call back into the
	// framework for the threadpool/data read/all kinda of stuff it shouldn't do on a
	// half-destroyed framework
	audioShutdown();
	LogInfo("Stopping threadpool");
	p->threadPool.reset();
	LogInfo("Clearing stages");
	p->ProgramStages.clear();
	LogInfo("Saving config");
	if (config().getBool("Config.Save"))
		config().save();

	LogInfo("Shutdown");
	// Make sure we destroy the data implementation before the renderer to ensure any possibly
	// cached images are already destroyed
	this->data.reset();
	if (createWindow)
	{
		displayShutdown();
	}
	LogInfo("SDL shutdown");
	PHYSFS_deinit();
	SDL_Quit();
	instance = nullptr;
}

Framework &Framework::getInstance()
{
	if (!instance)
	{
		LogError("Framework::getInstance() called with no live Framework");
	}
	return *instance;
}
Framework *Framework::tryGetInstance() { return instance; }

void Framework::run(sp<Stage> initialStage)
{
	size_t frameCount = Options::frameLimit.get();
	if (!createWindow)
	{
		LogError("Trying to run framework without window");
		return;
	}
	size_t frame = 0;
	TRACE_FN;
	LogInfo("Program loop started");

	auto target_frame_duration =
	    std::chrono::duration<int64_t, std::micro>(1000000 / Options::targetFPS.get());

	p->ProgramStages.push(initialStage);

	this->renderer->setPalette(this->data->loadPalette("xcom3/ufodata/pal_06.dat"));
	auto expected_frame_time = std::chrono::steady_clock::now();

	bool frame_time_limited_warning_shown = false;

	while (!p->quitProgram)
	{
		auto frame_time_now = std::chrono::steady_clock::now();
		if (expected_frame_time > frame_time_now)
		{
			TraceObj sleepTrace{"Sleep"};
			auto time_to_sleep = expected_frame_time - frame_time_now;
			auto time_to_sleep_us =
			    std::chrono::duration_cast<std::chrono::microseconds>(time_to_sleep);
			LogDebug("sleeping for %d us", time_to_sleep_us.count());
			std::this_thread::sleep_for(time_to_sleep);
			continue;
		}
		expected_frame_time += target_frame_duration;
		frame++;
		TraceObj obj{"Frame", {{"frame", Strings::fromInteger(frame)}}};

		if (!frame_time_limited_warning_shown &&
		    frame_time_now > expected_frame_time + 5 * target_frame_duration)
		{
			frame_time_limited_warning_shown = true;
			LogWarning("Over 5 frames behind - likely vsync limited?");
		}

		processEvents();

		if (p->ProgramStages.isEmpty())
		{
			break;
		}
		{
			TraceObj updateObj("Update");
			p->ProgramStages.current()->update();
		}

		for (StageCmd cmd : stageCommands)
		{
			switch (cmd.cmd)
			{
				case StageCmd::Command::CONTINUE:
					break;
				case StageCmd::Command::REPLACE:
					p->ProgramStages.pop();
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::REPLACEALL:
					p->ProgramStages.clear();
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::PUSH:
					p->ProgramStages.push(cmd.nextStage);
					break;
				case StageCmd::Command::POP:
					p->ProgramStages.pop();
					break;
				case StageCmd::Command::QUIT:
					p->quitProgram = true;
					p->ProgramStages.clear();
					break;
			}
			if (p->quitProgram)
			{
				break;
			}
		}
		stageCommands.clear();

		auto surface = p->scaleSurface ? p->scaleSurface : p->defaultSurface;
		RendererSurfaceBinding b(*this->renderer, surface);
		{
			TraceObj objClear{"clear"};
			this->renderer->clear();
		}
		if (!p->ProgramStages.isEmpty())
		{
			TraceObj updateObj("Render");
			p->ProgramStages.current()->render();
			if (p->toolTipImage)
			{
				renderer->draw(p->toolTipImage, p->toolTipPosition);
			}
			this->cursor->render();
			if (p->scaleSurface)
			{
				RendererSurfaceBinding scaleBind(*this->renderer, p->defaultSurface);
				TraceObj objClear{"clear scale"};
				this->renderer->clear();
				this->renderer->drawScaled(p->scaleSurface, {0, 0}, p->windowSize);
			}
			{
				TraceObj flipObj("Flip");
				this->renderer->flush();
				this->renderer->newFrame();
				SDL_GL_SwapWindow(p->window);
			}
		}
		if (frameCount && frame == frameCount)
		{
			LogWarning("Quitting hitting frame count limit of %llu", (unsigned long long)frame);
			p->quitProgram = true;
		}
	}
}

void Framework::processEvents()
{
	TRACE_FN;
	if (p->ProgramStages.isEmpty())
	{
		p->quitProgram = true;
		return;
	}

	// TODO: Consider threading the translation
	translateSdlEvents();

	while (p->eventQueue.size() > 0 && !p->ProgramStages.isEmpty())
	{
		up<Event> e;
		{
			std::lock_guard<std::mutex> l(p->eventQueueLock);
			e = std::move(p->eventQueue.front());
			p->eventQueue.pop_front();
		}
		if (!e)
		{
			LogError("Invalid event on queue");
			continue;
		}
		this->cursor->eventOccured(e.get());
		if (e->type() == EVENT_KEY_DOWN)
		{
			if (e->keyboard().KeyCode == SDLK_PRINTSCREEN)
			{
				int screenshotId = 0;
				UString screenshotName;
				do
				{
					screenshotName = format("screenshot%03d.png", screenshotId);
					screenshotId++;
				} while (fs::exists(fs::path(screenshotName.str())));
				LogWarning("Writing screenshot to \"%s\"", screenshotName);
				if (!p->defaultSurface->rendererPrivateData)
				{
					LogWarning("No renderer data on surface - nothing drawn yet?");
				}
				else
				{
					auto img = p->defaultSurface->rendererPrivateData->readBack();
					if (!img)
					{
						LogWarning("No image returned");
					}
					else
					{
						this->threadPoolTaskEnqueue([img, screenshotName] {
							auto ret = fw().data->writeImage(screenshotName, img);
							if (!ret)
							{
								LogWarning("Failed to write screenshot");
							}
							else
							{
								LogWarning("Wrote screenshot to \"%s\"", screenshotName);
							}
						});
					}
				}
			}
		}
		switch (e->type())
		{
			case EVENT_WINDOW_CLOSED:
				shutdownFramework();
				return;
			default:
				p->ProgramStages.current()->eventOccurred(e.get());
				break;
		}
	}
	/* Drop any events left in the list, as it's possible an event caused the last stage to pop
	 * with events outstanding, but they can safely be ignored as we're quitting anyway */
	{
		std::lock_guard<std::mutex> l(p->eventQueueLock);
		p->eventQueue.clear();
	}
}

void Framework::pushEvent(up<Event> e)
{
	std::lock_guard<std::mutex> l(p->eventQueueLock);
	p->eventQueue.push_back(std::move(e));
}

void Framework::pushEvent(Event *e) { this->pushEvent(up<Event>(e)); }

void Framework::translateSdlEvents()
{
	SDL_Event e;
	Event *fwE;
	bool touch_events_enabled = Options::optionEnableTouchEvents.get();

	// FIXME: That's not the right way to figure out the primary finger!
	int primaryFingerID = -1;
	if (SDL_GetNumTouchDevices())
	{
		SDL_Finger *primaryFinger = SDL_GetTouchFinger(SDL_GetTouchDevice(0), 0);
		if (primaryFinger)
		{
			primaryFingerID = primaryFinger->id;
		}
	}

	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
			case SDL_QUIT:
				fwE = new DisplayEvent(EVENT_WINDOW_CLOSED);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				// FIXME: Do nothing?
				break;
			case SDL_KEYDOWN:
				fwE = new KeyboardEvent(EVENT_KEY_DOWN);
				fwE->keyboard().KeyCode = e.key.keysym.sym;
				fwE->keyboard().ScanCode = e.key.keysym.scancode;
				fwE->keyboard().Modifiers = e.key.keysym.mod;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_KEYUP:
				fwE = new KeyboardEvent(EVENT_KEY_UP);
				fwE->keyboard().KeyCode = e.key.keysym.sym;
				fwE->keyboard().ScanCode = e.key.keysym.scancode;
				fwE->keyboard().Modifiers = e.key.keysym.mod;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTINPUT:
				fwE = new TextEvent();
				fwE->text().Input = e.text.text;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTEDITING:
				// FIXME: Do nothing?
				break;
			case SDL_MOUSEMOTION:
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				fwE->mouse().X = e.motion.x;
				fwE->mouse().Y = e.motion.y;
				fwE->mouse().DeltaX = e.motion.xrel;
				fwE->mouse().DeltaY = e.motion.yrel;
				fwE->mouse().WheelVertical = 0;   // These should be handled
				fwE->mouse().WheelHorizontal = 0; // in a separate event
				fwE->mouse().Button = e.motion.state;
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEWHEEL:
				// FIXME: Check these values for sanity
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				// Since I'm using some variables that are not used anywhere else,
				// this code should be in its own small block.
				{
					int mx, my;
					fwE->mouse().Button = SDL_GetMouseState(&mx, &my);
					fwE->mouse().X = mx;
					fwE->mouse().Y = my;
					fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
					fwE->mouse().DeltaY = 0;
					fwE->mouse().WheelVertical = e.wheel.y;
					fwE->mouse().WheelHorizontal = e.wheel.x;
				}
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONDOWN:
				fwE = new MouseEvent(EVENT_MOUSE_DOWN);
				fwE->mouse().X = e.button.x;
				fwE->mouse().Y = e.button.y;
				fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->mouse().DeltaY = 0;
				fwE->mouse().WheelVertical = 0;
				fwE->mouse().WheelHorizontal = 0;
				fwE->mouse().Button = SDL_BUTTON(e.button.button);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONUP:
				fwE = new MouseEvent(EVENT_MOUSE_UP);
				fwE->mouse().X = e.button.x;
				fwE->mouse().Y = e.button.y;
				fwE->mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->mouse().DeltaY = 0;
				fwE->mouse().WheelVertical = 0;
				fwE->mouse().WheelHorizontal = 0;
				fwE->mouse().Button = SDL_BUTTON(e.button.button);
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERDOWN:
				if (!touch_events_enabled)
					break;
				fwE = new FingerEvent(EVENT_FINGER_DOWN);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId == primaryFingerID; // FIXME: Try to remember the ID of
				                                           // the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERUP:
				if (!touch_events_enabled)
					break;
				fwE = new FingerEvent(EVENT_FINGER_UP);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId == primaryFingerID; // FIXME: Try to remember the ID of
				                                           // the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERMOTION:
				if (!touch_events_enabled)
					break;
				fwE = new FingerEvent(EVENT_FINGER_MOVE);
				fwE->finger().X = static_cast<int>(e.tfinger.x * displayGetWidth());
				fwE->finger().Y = static_cast<int>(e.tfinger.y * displayGetHeight());
				fwE->finger().DeltaX = static_cast<int>(e.tfinger.dx * displayGetWidth());
				fwE->finger().DeltaY = static_cast<int>(e.tfinger.dy * displayGetHeight());
				fwE->finger().Id = e.tfinger.fingerId;
				fwE->finger().IsPrimary =
				    e.tfinger.fingerId == primaryFingerID; // FIXME: Try to remember the ID of
				                                           // the first touching finger!
				pushEvent(up<Event>(fwE));
				break;
			case SDL_WINDOWEVENT:
				// Window events get special treatment
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						// FIXME: Do we care about SDL_WINDOWEVENT_SIZE_CHANGED?
						fwE = new DisplayEvent(EVENT_WINDOW_RESIZE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						fwE->display().Width = e.window.data1;
						fwE->display().Height = e.window.data2;
						fwE->display().Active = true;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_LEAVE:
						// FIXME: Check if we should react this way for each of those events
						// FIXME: Check if we're missing some of the events
						fwE = new DisplayEvent(EVENT_WINDOW_DEACTIVATE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->display().Width),
						                  &(fwE->display().Height));
						fwE->display().Active = false;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_EXPOSED:
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_ENTER:
						// FIXME: Should we handle all these events as "aaand we're back"
						// events?
						fwE = new DisplayEvent(EVENT_WINDOW_ACTIVATE);
						fwE->display().X = 0;
						fwE->display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->display().Width),
						                  &(fwE->display().Height));
						fwE->display().Active = false;
						pushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_CLOSE:
						// Closing a window will be a "quit" event.
						e.type = SDL_QUIT;
						SDL_PushEvent(&e);
						break;
				}
				break;
			default:
				break;
		}
	}
}

void Framework::shutdownFramework()
{
	LogInfo("Shutdown framework");
	p->ProgramStages.clear();
	p->quitProgram = true;
}

void Framework::displayInitialise()
{
	if (!this->createWindow)
	{
		return;
	}
	TRACE_FN;
	LogInfo("Init display");
	int display_flags = SDL_WINDOW_OPENGL;
#ifdef OPENAPOC_GLES
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
#ifdef SDL_OPENGL_CORE
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

#endif
#ifdef DEBUG_RENDERER
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif
	// Request context version 3.0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	// Request RGBA8888 - change if needed
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	SDL_GL_SetSwapInterval(Options::swapInterval.get());

	int scrW = Options::screenWidthOption.get();
	int scrH = Options::screenHeightOption.get();
	bool scrFS = Options::screenFullscreenOption.get();

	if (scrW < 640 || scrH < 480)
	{
		LogError(
		    "Requested display size of {%d,%d} is lower than {640,480} and probably won't work",
		    scrW, scrH);
	}

	if (scrFS)
	{
		display_flags |= SDL_WINDOW_FULLSCREEN;
	}

	p->window = SDL_CreateWindow("OpenApoc", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scrW,
	                             scrH, display_flags);

	if (!p->window)
	{
		LogError("Failed to create window \"%s\"", SDL_GetError());
		exit(1);
	}

	p->context = SDL_GL_CreateContext(p->window);
	if (!p->context)
	{
		LogWarning("Could not create GL context! [SDLError: %s]", SDL_GetError());
		LogWarning("Attempting to create context by lowering the requested version");
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
		p->context = SDL_GL_CreateContext(p->window);
		if (!p->context)
		{
			LogError("Failed to create GL context! [SDLerror: %s]", SDL_GetError());
			SDL_DestroyWindow(p->window);
			exit(1);
		}
	}
	// Output the context parameters
	LogInfo("Created OpenGL context, parameters:");
	int value;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, &value);
	UString profileType;
	switch (value)
	{
		case SDL_GL_CONTEXT_PROFILE_ES:
			profileType = "ES";
			break;
		case SDL_GL_CONTEXT_PROFILE_CORE:
			profileType = "Core";
			break;
		case SDL_GL_CONTEXT_PROFILE_COMPATIBILITY:
			profileType = "Compatibility";
			break;
		default:
			profileType = "Unknown";
	}
	LogInfo("  Context profile: %s", profileType);
	int ctxMajor, ctxMinor;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &ctxMajor);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &ctxMinor);
	LogInfo("  Context version: %d.%d", ctxMajor, ctxMinor);
	int bitsRed, bitsGreen, bitsBlue, bitsAlpha;
	SDL_GL_GetAttribute(SDL_GL_RED_SIZE, &bitsRed);
	SDL_GL_GetAttribute(SDL_GL_GREEN_SIZE, &bitsGreen);
	SDL_GL_GetAttribute(SDL_GL_BLUE_SIZE, &bitsBlue);
	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &bitsAlpha);
	LogInfo("  RGBA bits: %d-%d-%d-%d", bitsRed, bitsGreen, bitsBlue, bitsAlpha);
	SDL_GL_MakeCurrent(p->window, p->context); // for good measure?
	SDL_ShowCursor(SDL_DISABLE);

	p->registeredRenderers["GLES_3_0"].reset(getGLES30RendererFactory());
#ifndef __ANDROID__ // GL2 is not available on Android
	p->registeredRenderers["GL_2_0"].reset(getGL20RendererFactory());
#endif

	for (auto &rendererName : Options::renderersOption.get().split(':'))
	{
		auto rendererFactory = p->registeredRenderers.find(rendererName);
		if (rendererFactory == p->registeredRenderers.end())
		{
			LogInfo("Renderer \"%s\" not in supported list", rendererName);
			continue;
		}
		Renderer *r = rendererFactory->second->create();
		if (!r)
		{
			LogInfo("Renderer \"%s\" failed to init", rendererName);
			continue;
		}
		this->renderer.reset(r);
		LogInfo("Using renderer: %s", this->renderer->getName());
		break;
	}
	if (!this->renderer)
	{
		LogError("No functional renderer found");
		abort();
	}
	this->p->defaultSurface = this->renderer->getDefaultSurface();

	int width, height;
	SDL_GetWindowSize(p->window, &width, &height);
	p->windowSize = {width, height};

	// FIXME: Scale is currently stored as an integer in 1/100 units (ie 100 is 1.0 == same
	// size)
	int scaleX = Options::screenScaleXOption.get();
	int scaleY = Options::screenScaleYOption.get();

	if (scaleX != 100 || scaleY != 100)
	{
		float scaleXFloat = (float)scaleX / 100.0f;
		float scaleYFloat = (float)scaleY / 100.0f;
		p->displaySize.x = (int)((float)p->windowSize.x * scaleXFloat);
		p->displaySize.y = (int)((float)p->windowSize.y * scaleYFloat);
		if (p->displaySize.x < 640 || p->displaySize.y < 480)
		{
			LogWarning("Requested scaled size of %s is lower than {640,480} and probably "
			           "won't work, so forcing 640x480",
			           p->displaySize.x);
			p->displaySize.x = std::max(640, p->displaySize.x);
			p->displaySize.y = std::max(480, p->displaySize.y);
		}
		LogInfo("Scaling from %s to %s", p->displaySize, p->windowSize);
		p->scaleSurface = mksp<Surface>(p->displaySize);
	}
	else
	{
		p->displaySize = p->windowSize;
	}
	this->cursor.reset(new ApocCursor(this->data->loadPalette("xcom3/tacdata/tactical.pal")));
}

void Framework::displayShutdown()
{
	this->cursor.reset();
	if (!p->window)
	{
		return;
	}
	TRACE_FN;
	LogInfo("Shutdown Display");
	p->defaultSurface.reset();
	renderer.reset();

	SDL_GL_DeleteContext(p->context);
	SDL_DestroyWindow(p->window);
}

int Framework::displayGetWidth() { return p->displaySize.x; }

int Framework::displayGetHeight() { return p->displaySize.y; }

Vec2<int> Framework::displayGetSize() { return p->displaySize; }

bool Framework::displayHasWindow() const
{
	if (createWindow == false)
		return false;
	if (!p->window)
		return false;
	return true;
}

void Framework::displaySetTitle(UString NewTitle)
{
	if (p->window)
	{
		SDL_SetWindowTitle(p->window, NewTitle.cStr());
	}
}

void Framework::displaySetIcon(sp<RGBImage> image)
{
	if (!p->window)
	{
		return;
	}
#ifdef _WIN32
	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWindowWMInfo(p->window, &info);
	HINSTANCE handle = GetModuleHandle(NULL);
	HICON icon = LoadIcon(handle, L"ALLEGRO_ICON");
	HWND hwnd = info.info.win.window;
	SetClassLongPtr(hwnd, GCLP_HICON, (LONG_PTR)icon);
#else
	RGBImageLock reader(image, ImageLockUse::Read);
	// TODO: Should set the pixels instead of using a void*
	SDL_Surface *surface = SDL_CreateRGBSurfaceFrom(reader.getData(), image->size.x, image->size.y,
	                                                32, 0, 0xF000, 0x0F00, 0x00F0, 0x000F);
	SDL_SetWindowIcon(p->window, surface);
	SDL_FreeSurface(surface);
#endif
}

void Framework::audioInitialise()
{
	TRACE_FN;
	LogInfo("Initialise Audio");

	p->registeredSoundBackends["SDLRaw"].reset(getSDLSoundBackend());
	p->registeredSoundBackends["null"].reset(getNullSoundBackend());

	for (auto &soundBackendName : Options::audioBackendsOption.get().split(':'))
	{
		auto backendFactory = p->registeredSoundBackends.find(soundBackendName);
		if (backendFactory == p->registeredSoundBackends.end())
		{
			LogInfo("Sound backend %s not in supported list", soundBackendName);
			continue;
		}
		SoundBackend *backend = backendFactory->second->create();
		if (!backend)
		{
			LogInfo("Sound backend %s failed to init", soundBackendName);
			continue;
		}
		this->soundBackend.reset(backend);
		LogInfo("Using sound backend %s", soundBackendName);
		break;
	}
	if (!this->soundBackend)
	{
		LogError("No functional sound backend found");
	}
	this->jukebox.reset(new JukeBoxImpl(*this));

	/* Setup initial gain */
	this->soundBackend->setGain(SoundBackend::Gain::Global,
	                            static_cast<float>(Options::audioGlobalGainOption.get()) / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Music,
	                            static_cast<float>(Options::audioMusicGainOption.get()) / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Sample,
	                            static_cast<float>(Options::audioSampleGainOption.get()) / 20.0f);
}

void Framework::audioShutdown()
{
	TRACE_FN;
	LogInfo("Shutdown Audio");
	this->jukebox.reset();
	this->soundBackend.reset();
}

sp<Stage> Framework::stageGetCurrent() { return p->ProgramStages.current(); }

sp<Stage> Framework::stageGetPrevious() { return p->ProgramStages.previous(); }

sp<Stage> Framework::stageGetPrevious(sp<Stage> From) { return p->ProgramStages.previous(From); }

void Framework::stageQueueCommand(const StageCmd &cmd) { stageCommands.emplace_back(cmd); }

ApocCursor &Framework::getCursor() { return *this->cursor; }

void Framework::textStartInput() { SDL_StartTextInput(); }

void Framework::textStopInput() { SDL_StopTextInput(); }

void Framework::toolTipStartTimer(up<Event> e)
{
	int delay = config().getInt("Options.Misc.ToolTipDelay");
	if (delay <= 0)
	{
		return;
	}
	// remove any pending timers
	toolTipStopTimer();
	p->toolTipTimerEvent = std::move(e);
	p->toolTipTimerId = SDL_AddTimer(
	    delay,
	    [](unsigned int interval, void *data) -> unsigned int {
		    fw().toolTipTimerCallback(interval, data);
		    // remove this sdl timer
		    return 0;
	    },
	    nullptr);
}
void Framework::toolTipStopTimer()
{
	if (p->toolTipTimerId)
	{
		SDL_RemoveTimer(p->toolTipTimerId);
		p->toolTipTimerId = 0;
	}
	p->toolTipTimerEvent.reset();
	p->toolTipImage.reset();
}

void Framework::toolTipTimerCallback(unsigned int interval [[maybe_unused]],
                                     void *data [[maybe_unused]])
{
	// the sdl timer will be removed, so we forget about
	// clear the timerid and reset the event
	pushEvent(std::move(p->toolTipTimerEvent));
	p->toolTipTimerId = 0;
}

void Framework::showToolTip(sp<Image> image, const Vec2<int> &position)
{
	p->toolTipImage = image;
	p->toolTipPosition = position;
}

UString Framework::textGetClipboard()
{
	UString str;
	char *text = SDL_GetClipboardText();
	if (text != nullptr)
	{
		str = text;
		SDL_free(text);
	}
	return str;
}

void Framework::threadPoolTaskEnqueue(std::function<void()> task) { p->threadPool->enqueue(task); }

void *Framework::getWindowHandle() const { return static_cast<void *>(p->window); }

void Framework::setupModDataPaths()
{
	auto mods = Options::modList.get().split(":");
	for (const auto &modString : mods)
	{
		LogWarning("loading mod \"%s\"", modString);
		auto modPath = Options::modPath.get() + "/" + modString;
		auto modInfo = ModInfo::getInfo(modPath);
		if (!modInfo)
		{
			LogError("Failed to load ModInfo for mod \"%s\"", modString);
			continue;
		}
		auto modDataPath = modPath + "/" + modInfo->getDataPath();
		LogWarning("Loaded modinfo for mod ID \"%s\"", modInfo->getID());
		if (modInfo->getDataPath() != "")
		{
			LogWarning("Appending data path \"%s\"", modDataPath);
			this->data->fs.addPath(modDataPath);
		}
	}
}

}; // namespace OpenApoc
