#include "framework/framework.h"
#include "framework/apocresources/cursor.h"
#include "framework/event.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include "framework/sound.h"
#include "framework/sound_interface.h"
#include "framework/trace.h"
#include "library/sp.h"
#include <SDL.h>
#include <iostream>
#include <string>

// SDL_syswm includes windows.h on windows, which does all kinds of polluting
// defines/namespace stuff, so try to avoid that
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <SDL_syswm.h>
// Use physfs to get prefs dir
#include <physfs.h>

// Boost locale for setting the system locale
// Disable automatic #pragma linking for boost - only enabled in msvc and that should provide boost
// symbols as part of the module that uses it
#define BOOST_ALL_NO_LIB
#include <boost/filesystem.hpp>
#include <boost/locale.hpp>

#ifdef OPENAPOC_GLES

#include "framework/render/gles20/EGLContext.h"

#endif /*OPENAPOC_GLES*/

using namespace OpenApoc;

namespace
{

#ifndef DATA_DIRECTORY
#define DATA_DIRECTORY "./data"
#endif

#ifndef RENDERERS
#ifdef _WIN32
#pragma message("WARNING: Using default renderer list")
#else
#warning RENDERERS not set - using default list
#endif
#define RENDERERS "GLES_3_0:GL_3_0:GL_2_0"
#endif

static std::map<UString, UString> defaultConfig = {
#ifdef PANDORA
    {"Visual.ScreenWidth", "800"},
    {"Visual.ScreenHeight", "480"},
    {"Visual.FullScreen", "true"},
#else
    {"Visual.ScreenWidth", "1600"},
    {"Visual.ScreenHeight", "900"},
    {"Visual.FullScreen", "false"},
#endif
    {"Language", ""},
    {"GameRules", "XCOMAPOC.XML"},
    {"Resource.LocalDataDir", "./data"},
    {"Resource.SystemDataDir", DATA_DIRECTORY},
    {"Resource.LocalCDPath", "./data/cd.iso"},
    {"Resource.SystemCDPath", DATA_DIRECTORY "/cd.iso"},
    {"Resource.SaveDataDir", "./saves"},
    {"Resource.SaveSkipPacking", "0"},
    {"Visual.Renderers", RENDERERS},
    {"Audio.Backends", "SDLRaw:null"},
    {"Audio.GlobalGain", "20"},
    {"Audio.SampleGain", "20"},
    {"Audio.MusicGain", "20"},
    {"Framework.ThreadPoolSize", "0"},
    {"Visual.ScaleX", "100"},
    {"Visual.ScaleY", "100"},
};
};

namespace OpenApoc
{

Framework *Framework::instance = nullptr;

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<sp<MusicTrack>> trackList;
	PlayMode mode;

  public:
	JukeBoxImpl(Framework &fw) : fw(fw), mode(PlayMode::Loop) {}
	virtual ~JukeBoxImpl() { this->stop(); }

	void play(std::vector<UString> tracks, PlayMode mode) override
	{
		this->trackList.clear();
		this->position = 0;
		this->mode = mode;
		for (auto &track : tracks)
		{
			auto musicTrack = fw.data->load_music(track);
			if (!musicTrack)
				LogError("Failed to load music track \"%s\" - skipping", track.c_str());
			else
				this->trackList.push_back(musicTrack);
		}
		this->progressTrack(this);
		this->fw.soundBackend->playMusic(progressTrack, this);
	}
	static void progressTrack(void *data)
	{
		JukeBoxImpl *jukebox = static_cast<JukeBoxImpl *>(data);
		if (jukebox->trackList.size() == 0)
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
		        jukebox->trackList[jukebox->position]->getName().c_str());
		jukebox->fw.soundBackend->setTrack(jukebox->trackList[jukebox->position]);

		jukebox->position++;
		if (jukebox->mode == PlayMode::Loop)
			jukebox->position = jukebox->position % jukebox->trackList.size();
	}
	void stop() override { fw.soundBackend->stopMusic(); }
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

	FrameworkPrivate()
	    : quitProgram(false), window(nullptr), context(0), displaySize(0, 0), windowSize(0, 0)
	{
	}
};

Framework::Framework(const UString programName, const std::vector<UString> cmdline,
                     bool createWindow)
    : p(new FrameworkPrivate), programName(programName), createWindow(createWindow)
{
	TRACE_FN;
	LogInfo("Starting framework");

	if (this->instance)
	{
		LogError("Multiple Framework instances created");
	}

	this->instance = this;

	if (PHYSFS_init(programName.c_str()) == 0)
	{
		PHYSFS_ErrorCode error = PHYSFS_getLastErrorCode();
		LogError("Failed to init code %i PHYSFS: %s", (int)error, PHYSFS_getErrorByCode(error));
	}
#ifdef ANDROID
	SDL_SetHint(SDL_HINT_ANDROID_SEPARATE_MOUSE_AND_TOUCH, "1");
#endif
	// Initialize subsystems separately?
	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO) < 0)
	{
		LogError("Cannot init SDL2");
		LogError("SDL error: %s", SDL_GetError());
		p->quitProgram = true;
		return;
	}
	LogInfo("Loading config\n");
	p->quitProgram = false;
	UString settingsPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	settingsPath += "/settings.cfg";
	Settings.reset(new ConfigFile(settingsPath, defaultConfig));

	for (auto &option : cmdline)
	{
		auto splitString = option.split('=');
		if (splitString.size() != 2)
		{
			LogError("Failed to parse command line option \"%s\" - ignoring", option.c_str());
			continue;
		}
		else
		{
			LogInfo("Setting option \"%s\" to \"%s\" from command line", splitString[0].c_str(),
			        splitString[1].c_str());
			Settings->set(splitString[0], splitString[1]);
		}
	}

	// This is always set, the default being an empty string (which correctly chooses 'system
	// langauge')
	auto desiredLanguageName = Settings->getString("Language");

	LogInfo("Setting up locale \"%s\"", desiredLanguageName.c_str());

	boost::locale::generator gen;

	std::vector<UString> resourcePaths;
	resourcePaths.push_back(Settings->getString("Resource.SystemCDPath"));
	resourcePaths.push_back(Settings->getString("Resource.LocalCDPath"));
	resourcePaths.push_back(Settings->getString("Resource.SystemDataDir"));
	resourcePaths.push_back(Settings->getString("Resource.LocalDataDir"));

	for (auto &path : resourcePaths)
	{
		auto langPath = path + "/languages";
		LogInfo("Adding \"%s\" to language path", langPath.c_str());
		gen.add_messages_path(langPath.str());
	}

	std::vector<UString> translationDomains = {"paedia_string", "ufo_string"};
	for (auto &domain : translationDomains)
	{
		LogInfo("Adding \"%s\" to translation domains", domain.c_str());
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

	// Set the language in the config file
	Settings->set("Language", localeName);

	int threadPoolSize = Settings->getInt("Framework.ThreadPoolSize");
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

	LogInfo("Current working directory: \"%s\"", boost::filesystem::current_path().c_str());
	this->data.reset(new Data(resourcePaths));

	auto testFile = this->data->fs.open("MUSIC");
	if (!testFile)
	{
		LogError("Failed to open \"music\" from the CD - likely the cd couldn't be loaded or paths "
		         "are incorrect if using an extracted CD image");
	}

	auto testFile2 = this->data->fs.open("FileDoesntExist");
	if (testFile2)
	{
		LogError("Succeded in opening \"FileDoesntExist\" - either you have the weirdest filename "
		         "preferences or something is wrong");
	}
	srand(static_cast<unsigned int>(SDL_GetTicks()));

	if (createWindow)
	{
		Display_Initialise();
		Audio_Initialise();
	}
}

Framework::~Framework()
{
	TRACE_FN;
	LogInfo("Destroying framework");
	p->ProgramStages.Clear();
	LogInfo("Saving config");
	SaveSettings();

	LogInfo("Shutdown");
	if (createWindow)
	{
		Display_Shutdown();
		Audio_Shutdown();
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

void Framework::Run(sp<Stage> initialStage, size_t frameCount)
{
	if (!createWindow)
	{
		LogError("Trying to run framework without window");
		return;
	}
	size_t frame = 0;
	TRACE_FN;
	LogInfo("Program loop started");

	p->ProgramStages.Push(initialStage);

	this->renderer->setPalette(this->data->load_palette("xcom3/ufodata/PAL_06.DAT"));

	while (!p->quitProgram)
	{
		frame++;
		TraceObj obj{"Frame", {{"frame", Strings::FromInteger(frame)}}};

		ProcessEvents();

		StageCmd cmd;
		if (p->ProgramStages.IsEmpty())
		{
			break;
		}
		{
			TraceObj updateObj("Update");
			p->ProgramStages.Current()->Update(&cmd);
		}
		switch (cmd.cmd)
		{
			case StageCmd::Command::CONTINUE:
				break;
			case StageCmd::Command::REPLACE:
				p->ProgramStages.Pop();
				p->ProgramStages.Push(cmd.nextStage);
				break;
			case StageCmd::Command::REPLACEALL:
				p->ProgramStages.Clear();
				p->ProgramStages.Push(cmd.nextStage);
				break;
			case StageCmd::Command::PUSH:
				p->ProgramStages.Push(cmd.nextStage);
				break;
			case StageCmd::Command::POP:
				p->ProgramStages.Pop();
				break;
			case StageCmd::Command::QUIT:
				p->quitProgram = true;
				p->ProgramStages.Clear();
				break;
		}
		auto surface = p->scaleSurface ? p->scaleSurface : p->defaultSurface;
		RendererSurfaceBinding b(*this->renderer, surface);
		{
			TraceObj objClear{"clear"};
			this->renderer->clear();
		}
		if (!p->ProgramStages.IsEmpty())
		{
			TraceObj updateObj("Render");
			p->ProgramStages.Current()->Render();
			this->cursor->Render();
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
#ifndef OPENAPOC_GLES
				SDL_GL_SwapWindow(p->window);
#else
#ifdef _MSC_VER
				GLContext::GetInstance()->Swap();
#else
				SDL_GL_SwapWindow(p->window);
#endif
#endif
			}
		}
		if (frameCount && frame == frameCount)
		{
			LogWarning("Quitting hitting frame count limit of %llu", (unsigned long long)frame);
			p->quitProgram = true;
		}
	}
}

void Framework::ProcessEvents()
{
	TRACE_FN;
	if (p->ProgramStages.IsEmpty())
	{
		p->quitProgram = true;
		return;
	}

	// TODO: Consider threading the translation
	TranslateSDLEvents();

	while (p->eventQueue.size() > 0 && !p->ProgramStages.IsEmpty())
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
		this->cursor->EventOccured(e.get());
		if (e->Type() == EVENT_KEY_DOWN)
		{
			if (e->Keyboard().KeyCode == SDLK_F5)
			{
				UString screenshotName = "screenshot.png";
				LogWarning("Writing screenshot to \"%s\"", screenshotName.c_str());
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
						auto ret = data->write_image(screenshotName, img);
						if (!ret)
						{
							LogWarning("Failed to write screenshot");
						}
						else
						{
							LogWarning("Wrote screenshot to \"%s\"", screenshotName.c_str());
						}
					}
				}
			}
		}
		switch (e->Type())
		{
			case EVENT_WINDOW_CLOSED:
				ShutdownFramework();
				return;
			default:
				p->ProgramStages.Current()->EventOccurred(e.get());
				break;
		}
	}
	/* Drop any events left in the list, as it's possible an event caused the last stage to pop with
	 * events outstanding, but they can safely be ignored as we're quitting anyway */
	{
		std::lock_guard<std::mutex> l(p->eventQueueLock);
		p->eventQueue.clear();
	}
}

void Framework::PushEvent(up<Event> e)
{
	std::lock_guard<std::mutex> l(p->eventQueueLock);
	p->eventQueue.push_back(std::move(e));
}

void Framework::PushEvent(Event *e) { this->PushEvent(up<Event>(e)); }

void Framework::TranslateSDLEvents()
{
	SDL_Event e;
	Event *fwE;

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
				PushEvent(up<Event>(fwE));
				break;
			case SDL_JOYDEVICEADDED:
			case SDL_JOYDEVICEREMOVED:
				// FIXME: Do nothing?
				break;
			case SDL_KEYDOWN:
				fwE = new KeyboardEvent(EVENT_KEY_DOWN);
				fwE->Keyboard().KeyCode = e.key.keysym.sym;
				fwE->Keyboard().UniChar = e.key.keysym.sym;
				fwE->Keyboard().Modifiers = e.key.keysym.mod;
				PushEvent(up<Event>(fwE));
				break;
			case SDL_KEYUP:
				fwE = new KeyboardEvent(EVENT_KEY_UP);
				fwE->Keyboard().KeyCode = e.key.keysym.sym;
				fwE->Keyboard().UniChar = e.key.keysym.sym;
				fwE->Keyboard().Modifiers = e.key.keysym.mod;
				PushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTINPUT:
				fwE = new TextEvent();
				fwE->Text().Input = e.text.text;
				PushEvent(up<Event>(fwE));
				break;
			case SDL_TEXTEDITING:
				// FIXME: Do nothing?
				break;
			case SDL_MOUSEMOTION:
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				fwE->Mouse().X = e.motion.x;
				fwE->Mouse().Y = e.motion.y;
				fwE->Mouse().DeltaX = e.motion.xrel;
				fwE->Mouse().DeltaY = e.motion.yrel;
				fwE->Mouse().WheelVertical = 0;   // These should be handled
				fwE->Mouse().WheelHorizontal = 0; // in a separate event
				fwE->Mouse().Button = e.motion.state;
				PushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEWHEEL:
				// FIXME: Check these values for sanity
				fwE = new MouseEvent(EVENT_MOUSE_MOVE);
				// Since I'm using some variables that are not used anywhere else,
				// this code should be in its own small block.
				{
					int mx, my;
					fwE->Mouse().Button = SDL_GetMouseState(&mx, &my);
					fwE->Mouse().X = mx;
					fwE->Mouse().Y = my;
					fwE->Mouse().DeltaX = 0; // FIXME: This might cause problems?
					fwE->Mouse().DeltaY = 0;
					fwE->Mouse().WheelVertical = e.wheel.y;
					fwE->Mouse().WheelHorizontal = e.wheel.x;
				}
				PushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONDOWN:
				fwE = new MouseEvent(EVENT_MOUSE_DOWN);
				fwE->Mouse().X = e.button.x;
				fwE->Mouse().Y = e.button.y;
				fwE->Mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->Mouse().DeltaY = 0;
				fwE->Mouse().WheelVertical = 0;
				fwE->Mouse().WheelHorizontal = 0;
				fwE->Mouse().Button = SDL_BUTTON(e.button.button);
				PushEvent(up<Event>(fwE));
				break;
			case SDL_MOUSEBUTTONUP:
				fwE = new MouseEvent(EVENT_MOUSE_UP);
				fwE->Mouse().X = e.button.x;
				fwE->Mouse().Y = e.button.y;
				fwE->Mouse().DeltaX = 0; // FIXME: This might cause problems?
				fwE->Mouse().DeltaY = 0;
				fwE->Mouse().WheelVertical = 0;
				fwE->Mouse().WheelHorizontal = 0;
				fwE->Mouse().Button = SDL_BUTTON(e.button.button);
				PushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERDOWN:
				fwE = new FingerEvent(EVENT_FINGER_DOWN);
				fwE->Finger().X = static_cast<int>(e.tfinger.x * Display_GetWidth());
				fwE->Finger().Y = static_cast<int>(e.tfinger.y * Display_GetHeight());
				fwE->Finger().DeltaX = static_cast<int>(e.tfinger.dx * Display_GetWidth());
				fwE->Finger().DeltaY = static_cast<int>(e.tfinger.dy * Display_GetHeight());
				fwE->Finger().Id = e.tfinger.fingerId;
				fwE->Finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				PushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERUP:
				fwE = new FingerEvent(EVENT_FINGER_UP);
				fwE->Finger().X = static_cast<int>(e.tfinger.x * Display_GetWidth());
				fwE->Finger().Y = static_cast<int>(e.tfinger.y * Display_GetHeight());
				fwE->Finger().DeltaX = static_cast<int>(e.tfinger.dx * Display_GetWidth());
				fwE->Finger().DeltaY = static_cast<int>(e.tfinger.dy * Display_GetHeight());
				fwE->Finger().Id = e.tfinger.fingerId;
				fwE->Finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				PushEvent(up<Event>(fwE));
				break;
			case SDL_FINGERMOTION:
				fwE = new FingerEvent(EVENT_FINGER_MOVE);
				fwE->Finger().X = static_cast<int>(e.tfinger.x * Display_GetWidth());
				fwE->Finger().Y = static_cast<int>(e.tfinger.y * Display_GetHeight());
				fwE->Finger().DeltaX = static_cast<int>(e.tfinger.dx * Display_GetWidth());
				fwE->Finger().DeltaY = static_cast<int>(e.tfinger.dy * Display_GetHeight());
				fwE->Finger().Id = e.tfinger.fingerId;
				fwE->Finger().IsPrimary =
				    e.tfinger.fingerId ==
				    primaryFingerID; // FIXME: Try to remember the ID of the first touching finger!
				PushEvent(up<Event>(fwE));
				break;
			case SDL_WINDOWEVENT:
				// Window events get special treatment
				switch (e.window.event)
				{
					case SDL_WINDOWEVENT_RESIZED:
						// FIXME: Do we care about SDL_WINDOWEVENT_SIZE_CHANGED?
						fwE = new DisplayEvent(EVENT_WINDOW_RESIZE);
						fwE->Display().X = 0;
						fwE->Display().Y = 0;
						fwE->Display().Width = e.window.data1;
						fwE->Display().Height = e.window.data2;
						fwE->Display().Active = true;
						break;
					case SDL_WINDOWEVENT_HIDDEN:
					case SDL_WINDOWEVENT_MINIMIZED:
					case SDL_WINDOWEVENT_LEAVE:
						// FIXME: Check if we should react this way for each of those events
						// FIXME: Check if we're missing some of the events
						fwE = new DisplayEvent(EVENT_WINDOW_DEACTIVATE);
						fwE->Display().X = 0;
						fwE->Display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->Display().Width),
						                  &(fwE->Display().Height));
						fwE->Display().Active = false;
						PushEvent(up<Event>(fwE));
						break;
					case SDL_WINDOWEVENT_SHOWN:
					case SDL_WINDOWEVENT_EXPOSED:
					case SDL_WINDOWEVENT_RESTORED:
					case SDL_WINDOWEVENT_ENTER:
						// FIXME: Should we handle all these events as "aaand we're back" events?
						fwE = new DisplayEvent(EVENT_WINDOW_ACTIVATE);
						fwE->Display().X = 0;
						fwE->Display().Y = 0;
						// FIXME: Is this even necessary?
						SDL_GetWindowSize(p->window, &(fwE->Display().Width),
						                  &(fwE->Display().Height));
						fwE->Display().Active = false;
						PushEvent(up<Event>(fwE));
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

void Framework::ShutdownFramework()
{
	LogInfo("Shutdown framework");
	p->ProgramStages.Clear();
	p->quitProgram = true;
}

void Framework::SaveSettings()
{
	// Just to keep the filename consistant
	UString settingsPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	settingsPath += "/settings.cfg";
	Settings->save(settingsPath);
}

void Framework::Display_Initialise()
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
	// Request RGBA5551 - change if needed
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 1);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int scrW = Settings->getInt("Visual.ScreenWidth");
	int scrH = Settings->getInt("Visual.ScreenHeight");
	bool scrFS = Settings->getBool("Visual.FullScreen");

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
		LogError("Failed to create window");
		;
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
	std::string profileType;
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
	LogInfo("  Context profile: %s", profileType.c_str());
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
	SDL_GL_SetSwapInterval(1);
	SDL_GL_MakeCurrent(p->window, p->context); // for good measure?
	SDL_ShowCursor(SDL_DISABLE);

	p->registeredRenderers["GLES_3_0"].reset(getGLES30RendererFactory());
	p->registeredRenderers["GL_3_0"].reset(getGL30RendererFactory());
	p->registeredRenderers["GL_2_0"].reset(getGL20RendererFactory());

	for (auto &rendererName : Settings->getString("Visual.Renderers").split(':'))
	{
		auto rendererFactory = p->registeredRenderers.find(rendererName);
		if (rendererFactory == p->registeredRenderers.end())
		{
			LogInfo("Renderer \"%s\" not in supported list", rendererName.c_str());
			continue;
		}
		Renderer *r = rendererFactory->second->create();
		if (!r)
		{
			LogInfo("Renderer \"%s\" failed to init", rendererName.c_str());
			continue;
		}
		this->renderer.reset(r);
		LogInfo("Using renderer: %s", this->renderer->getName().c_str());
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

	// FIXME: Scale is currently stored as an integer in 1/100 units (ie 100 is 1.0 == same size)
	int scaleX = Settings->getInt("Visual.ScaleX");
	int scaleY = Settings->getInt("Visual.ScaleY");

	if (scaleX != 100 || scaleY != 100)
	{
		float scaleXFloat = (float)scaleX / 100.0f;
		float scaleYFloat = (float)scaleY / 100.0f;
		p->displaySize.x = (int)((float)p->windowSize.x * scaleXFloat);
		p->displaySize.y = (int)((float)p->windowSize.y * scaleYFloat);
		if (p->displaySize.x < 640 || p->displaySize.y < 480)
		{
			LogWarning("Requested scaled size of {%d,%d} is lower than {640,480} and probably "
			           "won't work, so forcing 640x480",
			           p->displaySize.x, p->displaySize.y);
			p->displaySize.x = std::max(640, p->displaySize.x);
			p->displaySize.y = std::max(480, p->displaySize.y);
		}
		LogInfo("Scaling from {%d,%d} to {%d,%d}", p->displaySize.x, p->displaySize.y,
		        p->windowSize.x, p->windowSize.y);
		p->scaleSurface = mksp<Surface>(p->displaySize);
	}
	else
	{
		p->displaySize = p->windowSize;
	}
	this->cursor.reset(new ApocCursor(this->data->load_palette("xcom3/tacdata/TACTICAL.PAL")));
}

void Framework::Display_Shutdown()
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

int Framework::Display_GetWidth() { return p->displaySize.x; }

int Framework::Display_GetHeight() { return p->displaySize.y; }

Vec2<int> Framework::Display_GetSize() { return p->displaySize; }

void Framework::Display_SetTitle(UString NewTitle)
{
	if (p->window)
	{
		SDL_SetWindowTitle(p->window, NewTitle.c_str());
	}
}

void Framework::Display_SetIcon()
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
// TODO: Figure out how this works
// NOTE: this should be a call to SDL_SetWindowIcon(p->window, icon);
// where icon should be a SDL_Surface* with our desired icon.
#endif
}

void Framework::Audio_Initialise()
{
	TRACE_FN;
	LogInfo("Initialise Audio");

	p->registeredSoundBackends["SDLRaw"].reset(getSDLSoundBackend());
	p->registeredSoundBackends["null"].reset(getNullSoundBackend());

	for (auto &soundBackendName : Settings->getString("Audio.Backends").split(':'))
	{
		auto backendFactory = p->registeredSoundBackends.find(soundBackendName);
		if (backendFactory == p->registeredSoundBackends.end())
		{
			LogInfo("Sound backend %s not in supported list", soundBackendName.c_str());
			continue;
		}
		SoundBackend *backend = backendFactory->second->create();
		if (!backend)
		{
			LogInfo("Sound backend %s failed to init", soundBackendName.c_str());
			continue;
		}
		this->soundBackend.reset(backend);
		LogInfo("Using sound backend %s", soundBackendName.c_str());
		break;
	}
	if (!this->soundBackend)
	{
		LogError("No functional sound backend found");
	}
	this->jukebox.reset(new JukeBoxImpl(*this));

	/* Setup initial gain */
	this->soundBackend->setGain(SoundBackend::Gain::Global,
	                            static_cast<float>(this->Settings->getInt("Audio.GlobalGain")) /
	                                20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Music,
	                            static_cast<float>(this->Settings->getInt("Audio.MusicGain")) /
	                                20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Sample,
	                            static_cast<float>(this->Settings->getInt("Audio.SampleGain")) /
	                                20.0f);
}

void Framework::Audio_Shutdown()
{
	TRACE_FN;
	LogInfo("Shutdown Audio");
	this->jukebox.reset();
	this->soundBackend.reset();
}

sp<Stage> Framework::Stage_GetPrevious() { return p->ProgramStages.Previous(); }

sp<Stage> Framework::Stage_GetPrevious(sp<Stage> From) { return p->ProgramStages.Previous(From); }

void Framework::Stage_Push(sp<Stage> stage)
{
	LogAssert(stage);
	p->ProgramStages.Push(stage);
}

Vec2<int> Framework::getCursorPosition() { return this->cursor->getPosition(); }

void Framework::Text_StartInput() { SDL_StartTextInput(); }

void Framework::Text_StopInput() { SDL_StopTextInput(); }

}; // namespace OpenApoc
