
#include "framework/framework.h"
#include "game/boot.h"
#include "game/resources/gamecore.h"
#include "game/city/city.h"
#include "framework/renderer.h"
#include "framework/renderer_interface.h"
#include "framework/sound_interface.h"
#include "framework/sound.h"

#include <allegro5/allegro5.h>
#include <iostream>
#include <string>
#include <physfs.h>

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
#define RENDERERS "GL_3_0:GLES_2_0"
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
    {"Language", "en_gb"},
    {"GameRules", "XCOMAPOC.XML"},
    {"Resource.LocalDataDir", "./data"},
    {"Resource.SystemDataDir", DATA_DIRECTORY},
    {"Resource.LocalCDPath", "./data/cd.iso"},
    {"Resource.SystemCDPath", DATA_DIRECTORY "/cd.iso"},
    {"Visual.Renderers", RENDERERS},
    {"Audio.Backends", "allegro:null"},
    {"Audio.GlobalGain", "20"},
    {"Audio.SampleGain", "20"},
    {"Audio.MusicGain", "20"},
};

std::map<UString, std::unique_ptr<OpenApoc::RendererFactory>> *registeredRenderers = nullptr;
std::map<UString, std::unique_ptr<OpenApoc::SoundBackendFactory>> *registeredSoundBackends =
    nullptr;
};

namespace OpenApoc
{

void registerRenderer(RendererFactory *factory, UString name)
{
	if (!registeredRenderers) {
		registeredRenderers = new std::map<UString, std::unique_ptr<OpenApoc::RendererFactory>>();
	}
	registeredRenderers->emplace(name, std::unique_ptr<RendererFactory>(factory));
}

void registerSoundBackend(SoundBackendFactory *factory, UString name)
{
	if (!registeredSoundBackends) {
		registeredSoundBackends =
		    new std::map<UString, std::unique_ptr<OpenApoc::SoundBackendFactory>>();
	}
	registeredSoundBackends->emplace(name, std::unique_ptr<SoundBackendFactory>(factory));
}

class JukeBoxImpl : public JukeBox
{
	Framework &fw;
	unsigned int position;
	std::vector<std::shared_ptr<MusicTrack>> trackList;
	JukeBox::PlayMode mode;

  public:
	JukeBoxImpl(Framework &fw) : fw(fw), mode(JukeBox::PlayMode::Loop) {}
	virtual ~JukeBoxImpl() { this->stop(); }

	virtual void play(std::vector<UString> tracks, JukeBox::PlayMode mode) override
	{
		this->trackList.clear();
		this->position = 0;
		this->mode = mode;
		for (auto &track : tracks) {
			auto musicTrack = fw.data->load_music(track);
			if (!musicTrack)
				LogError("Failed to load music track \"%s\" - skipping", track.str().c_str());
			else {
				this->trackList.push_back(musicTrack);
			}
		}
		this->progressTrack(this);
	}
	static void progressTrack(void *data)
	{
		JukeBoxImpl *jukebox = static_cast<JukeBoxImpl *>(data);
		if (jukebox->trackList.size() == 0) {
			LogWarning("Trying to play empty jukebox");
			return;
		}
		if (jukebox->position >= jukebox->trackList.size()) {
			LogInfo("End of jukebox playlist");
			return;
		}
		jukebox->fw.soundBackend->playMusic(jukebox->trackList[jukebox->position], progressTrack,
		                                    jukebox);
		jukebox->position++;
		if (jukebox->mode == JukeBox::PlayMode::Loop) {
			jukebox->position = jukebox->position % jukebox->trackList.size();
		}
	}
	virtual void stop() override { fw.soundBackend->stopMusic(); }
};

class FrameworkPrivate
{
  private:
	friend class Framework;
	bool quitProgram;

	ALLEGRO_DISPLAY_MODE screenMode;
	ALLEGRO_DISPLAY *screen;

	ALLEGRO_EVENT_QUEUE *eventAllegro;
	std::list<Event *> eventQueue;
	ALLEGRO_MUTEX *eventMutex;

	StageStack ProgramStages;
	std::shared_ptr<Surface> defaultSurface;
};

Framework::Framework(const UString programName, const std::vector<UString> cmdline)
    : dumpEvents(false), replayEvents(false), p(new FrameworkPrivate), programName(programName)
{
	LogInfo("Starting framework");
	PHYSFS_init(programName.str().c_str());

	if (!al_init()) {
		LogError("Cannot init Allegro");
		p->quitProgram = true;
		return;
	}

	if (!al_install_keyboard() || !al_install_mouse()) {
		LogError(" Cannot init Allegro plugins");
		p->quitProgram = true;
		return;
	}

	LogInfo("Loading config\n");
	p->quitProgram = false;
	UString settingsPath(PHYSFS_getPrefDir(PROGRAM_ORGANISATION, PROGRAM_NAME));
	settingsPath += "/settings.cfg";
	Settings.reset(new ConfigFile(settingsPath, defaultConfig));

	for (auto &option : cmdline) {
		auto splitString = option.split('=');
		if (splitString.size() != 2) {
			LogError("Failed to parse command line option \"%s\" - ignoring", option.str().c_str());
			continue;
		} else if (splitString[0] == "--dumpevents") {
			if (this->replayEvents || this->dumpEvents) {
				LogError(
				    "Only one --dumpevents or --replayevents should be supplied, ignoring \"%s\"",
				    option.str().c_str());
			} else {
				this->eventStream.open(splitString[1].str(), std::ios::out);
				if (!this->eventStream) {
					LogError("Failed to open event file \"%s\" for writing - ignoring --dumpevents "
					         "option",
					         splitString[1].str().c_str());
				} else {
					this->dumpEvents = true;
					LogInfo("Dumping events to file \"%s\"", splitString[1].str().c_str());
				}
			}
		} else if (splitString[0] == "--replayevents") {
			if (this->replayEvents || this->dumpEvents) {
				LogError(
				    "Only one --dumpevents or --replayevents should be supplied, ignoring \"%s\"",
				    option.str().c_str());
			} else {
				this->eventStream.open(splitString[1].str(), std::ios::in);
				if (!this->eventStream) {
					LogError("Failed to open event file \"%s\" for reading - ignoring "
					         "--replayevents option",
					         splitString[1].str().c_str());
				} else {
					this->replayEvents = true;
					LogInfo("Replaying events from file \"%s\"", splitString[1].str().c_str());
				}
			}
		} else {
			LogInfo("Setting option \"%s\" to \"%s\" from command line",
			        splitString[0].str().c_str(), splitString[1].str().c_str());
			Settings->set(splitString[0], splitString[1]);
		}
	}

	std::vector<UString> resourcePaths;
	resourcePaths.push_back(Settings->getString("Resource.SystemCDPath"));
	resourcePaths.push_back(Settings->getString("Resource.LocalCDPath"));
	resourcePaths.push_back(Settings->getString("Resource.SystemDataDir"));
	resourcePaths.push_back(Settings->getString("Resource.LocalDataDir"));

	this->data.reset(new Data(*this, resourcePaths));

	auto testFile = this->data->load_file("MUSIC");
	if (!testFile) {
		LogError("Failed to open \"music\" from the CD - likely the cd couldn't be loaded or paths "
		         "are incorrect if using an extracted CD image");
	}

	auto testFile2 = this->data->load_file("FileDoesntExist");
	if (testFile2) {
		LogError("Succeded in opening \"FileDoesntExist\" - either you have the weirdest filename "
		         "preferences or something is wrong");
	}

	p->eventAllegro = al_create_event_queue();
	p->eventMutex = al_create_mutex_recursive();

	srand((unsigned int)al_get_time());

	Display_Initialise();
	Audio_Initialise();

	al_register_event_source(p->eventAllegro, al_get_display_event_source(p->screen));
	al_register_event_source(p->eventAllegro, al_get_keyboard_event_source());
	al_register_event_source(p->eventAllegro, al_get_mouse_event_source());
}

Framework::~Framework()
{
	LogInfo("Destroying framework");
	// Kill gamecore and program stages first, so any resources are cleaned before
	// allegro is de-inited
	state.reset();
	gamecore.reset();
	p->ProgramStages.Clear();
	LogInfo("Saving config");
	SaveSettings();

	LogInfo("Shutdown");
	Display_Shutdown();
	Audio_Shutdown();
	al_destroy_event_queue(p->eventAllegro);
	al_destroy_mutex(p->eventMutex);

	LogInfo("Allegro shutdown");
	al_uninstall_mouse();
	al_uninstall_keyboard();

	al_uninstall_system();
	PHYSFS_deinit();
}

void Framework::Run()
{
	LogInfo("Program loop started");

	p->ProgramStages.Push(std::make_shared<BootUp>(*this));

	this->renderer->setPalette(this->data->load_palette("xcom3/ufodata/PAL_06.DAT"));

	while (!p->quitProgram) {
		RendererSurfaceBinding b(*this->renderer, p->defaultSurface);
		this->renderer->clear();
		ProcessEvents();

		StageCmd cmd;
		if (p->ProgramStages.IsEmpty()) {
			break;
		}
		p->ProgramStages.Current()->Update(&cmd);
		switch (cmd.cmd) {
		case StageCmd::Command::CONTINUE:
			break;
		case StageCmd::Command::REPLACE:
			p->ProgramStages.Pop();
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
		if (!p->ProgramStages.IsEmpty()) {
			p->ProgramStages.Current()->Render();
			al_flip_display();
		}
		if (this->dumpEvents) {
			// insert END_OF_FRAME to mark the end of each frame
			auto e = new Event();
			e->Type = EVENT_END_OF_FRAME;
			DumpEvent(e);
			delete e;
		}
	}
}

void Framework::ReadRecordedEvents()
{
	std::string line;
	while (std::getline(this->eventStream, line)) {
		Event *e = new Event(UString(line));
		PushEvent(e);
		if (e->Type == EVENT_END_OF_FRAME) {
			break;
		}
	}
	if (!this->eventStream) {
		LogInfo("Reached end of reply, appending CLOSE");
		auto e = new Event();
		e->Type = EVENT_WINDOW_CLOSED;
		PushEvent(e);
	}
}

void Framework::ProcessEvents()
{
	if (p->ProgramStages.IsEmpty()) {
		p->quitProgram = true;
		return;
	}

	// Convert Allegro events before we process
	// TODO: Consider threading the translation
	if (this->replayEvents) {
		ReadRecordedEvents();
	} else {
		TranslateAllegroEvents();
	}

	al_lock_mutex(p->eventMutex);

	while (p->eventQueue.size() > 0 && !p->ProgramStages.IsEmpty()) {
		Event *e;
		e = p->eventQueue.front();
		p->eventQueue.pop_front();
		if (!e) {
			LogError("Invalid event on queue");
			continue;
		}
		if (this->dumpEvents) {
			DumpEvent(e);
		}
		switch (e->Type) {
		case EVENT_WINDOW_CLOSED:
			delete e;
			al_unlock_mutex(p->eventMutex);
			ShutdownFramework();
			return;
			break;
		default:
			p->ProgramStages.Current()->EventOccurred(e);
			break;
		}
		delete e;
	}

	al_unlock_mutex(p->eventMutex);
}

void Framework::PushEvent(Event *e)
{
	al_lock_mutex(p->eventMutex);
	p->eventQueue.push_back(e);
	al_unlock_mutex(p->eventMutex);
}

void Framework::DumpEvent(Event *e)
{
	assert(e);
	assert(this->eventStream);
	this->eventStream << std::dec;
	this->eventStream << e->toString().str() << "\n";
}

void Framework::TranslateAllegroEvents()
{
	ALLEGRO_EVENT e;
	Event *fwE;

	while (al_get_next_event(p->eventAllegro, &e)) {
		switch (e.type) {
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			fwE = new Event();
			fwE->Type = EVENT_WINDOW_CLOSED;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
			al_reconfigure_joysticks();
			break;
		case ALLEGRO_EVENT_TIMER:
			fwE = new Event();
			fwE->Type = EVENT_TIMER_TICK;
			fwE->Data.Timer.TimerObject = (void *)e.timer.source;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			fwE = new Event();
			fwE->Type = EVENT_KEY_DOWN;
			fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
			fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
			fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_KEY_UP:
			fwE = new Event();
			fwE->Type = EVENT_KEY_UP;
			fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
			fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
			fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_KEY_CHAR:
			fwE = new Event();
			fwE->Type = EVENT_KEY_PRESS;
			fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
			fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
			fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_MOUSE_AXES:
			fwE = new Event();
			fwE->Type = EVENT_MOUSE_MOVE;
			fwE->Data.Mouse.X = e.mouse.x;
			fwE->Data.Mouse.Y = e.mouse.y;
			fwE->Data.Mouse.DeltaX = e.mouse.dx;
			fwE->Data.Mouse.DeltaY = e.mouse.dy;
			fwE->Data.Mouse.WheelVertical = e.mouse.dz;
			fwE->Data.Mouse.WheelHorizontal = e.mouse.dw;
			fwE->Data.Mouse.Button = e.mouse.button;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
			fwE = new Event();
			fwE->Type = EVENT_MOUSE_DOWN;
			fwE->Data.Mouse.X = e.mouse.x;
			fwE->Data.Mouse.Y = e.mouse.y;
			fwE->Data.Mouse.DeltaX = e.mouse.dx;
			fwE->Data.Mouse.DeltaY = e.mouse.dy;
			fwE->Data.Mouse.WheelVertical = e.mouse.dz;
			fwE->Data.Mouse.WheelHorizontal = e.mouse.dw;
			fwE->Data.Mouse.Button = e.mouse.button;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
			fwE = new Event();
			fwE->Type = EVENT_MOUSE_UP;
			fwE->Data.Mouse.X = e.mouse.x;
			fwE->Data.Mouse.Y = e.mouse.y;
			fwE->Data.Mouse.DeltaX = e.mouse.dx;
			fwE->Data.Mouse.DeltaY = e.mouse.dy;
			fwE->Data.Mouse.WheelVertical = e.mouse.dz;
			fwE->Data.Mouse.WheelHorizontal = e.mouse.dw;
			fwE->Data.Mouse.Button = e.mouse.button;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_DISPLAY_RESIZE:
			fwE = new Event();
			fwE->Type = EVENT_WINDOW_RESIZE;
			fwE->Data.Display.X = 0;
			fwE->Data.Display.Y = 0;
			fwE->Data.Display.Width = al_get_display_width(p->screen);
			fwE->Data.Display.Height = al_get_display_height(p->screen);
			fwE->Data.Display.Active = true;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
			fwE = new Event();
			fwE->Type = EVENT_WINDOW_ACTIVATE;
			fwE->Data.Display.X = 0;
			fwE->Data.Display.Y = 0;
			fwE->Data.Display.Width = al_get_display_width(p->screen);
			fwE->Data.Display.Height = al_get_display_height(p->screen);
			fwE->Data.Display.Active = true;
			PushEvent(fwE);
			break;
		case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			fwE = new Event();
			fwE->Type = EVENT_WINDOW_DEACTIVATE;
			fwE->Data.Display.X = 0;
			fwE->Data.Display.Y = 0;
			fwE->Data.Display.Width = al_get_display_width(p->screen);
			fwE->Data.Display.Height = al_get_display_height(p->screen);
			fwE->Data.Display.Active = false;
			PushEvent(fwE);
			break;
		default:
			fwE = new Event();
			fwE->Type = EVENT_UNDEFINED;
			PushEvent(fwE);
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
	LogInfo("Init display");
	int display_flags = ALLEGRO_OPENGL;
#ifdef ALLEGRO_OPENGL_CORE
	display_flags |= ALLEGRO_OPENGL_CORE;
#endif

#if ALLEGRO_VERSION > 5 || (ALLEGRO_VERSION == 5 && ALLEGRO_SUB_VERSION >= 1)
	display_flags |= ALLEGRO_OPENGL_3_0 | ALLEGRO_PROGRAMMABLE_PIPELINE;
#endif

	int scrW = Settings->getInt("Visual.ScreenWidth");
	int scrH = Settings->getInt("Visual.ScreenHeight");
	bool scrFS = Settings->getBool("Visual.FullScreen");

	if (scrFS) {
		display_flags |= ALLEGRO_FULLSCREEN;
	}

	al_set_new_display_flags(display_flags);

	al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);

	p->screen = al_create_display(scrW, scrH);

	if (!p->screen) {
		LogError("Failed to create screen");
		;
		exit(1);
	}

	al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

	al_hide_mouse_cursor(p->screen);

	for (auto &rendererName : Settings->getString("Visual.Renderers").split(':')) {
		auto rendererFactory = registeredRenderers->find(rendererName);
		if (rendererFactory == registeredRenderers->end()) {
			LogInfo("Renderer \"%s\" not in supported list", rendererName.str().c_str());
			continue;
		}
		Renderer *r = rendererFactory->second->create();
		if (!r) {
			LogInfo("Renderer \"%s\" failed to init", rendererName.str().c_str());
			continue;
		}
		this->renderer.reset(r);
		LogInfo("Using renderer: %s", this->renderer->getName().str().c_str());
		break;
	}
	if (!this->renderer) {
		LogError("No functional renderer found");
		abort();
	}
	this->p->defaultSurface = this->renderer->getDefaultSurface();
}

void Framework::Display_Shutdown()
{
	LogInfo("Shutdown Display");
	p->defaultSurface.reset();
	renderer.reset();

	al_unregister_event_source(p->eventAllegro, al_get_display_event_source(p->screen));
	al_destroy_display(p->screen);
}

int Framework::Display_GetWidth() { return al_get_display_width(p->screen); }

int Framework::Display_GetHeight() { return al_get_display_height(p->screen); }

Vec2<int> Framework::Display_GetSize()
{
	return Vec2<int>(this->Display_GetWidth(), this->Display_GetHeight());
}

void Framework::Display_SetTitle(UString NewTitle)
{
#ifdef _WIN32
	al_set_app_name(NewTitle.str().c_str());
	al_set_window_title(p->screen, NewTitle.str().c_str());
#else
	al_set_app_name(NewTitle.str().c_str());
	al_set_window_title(p->screen, NewTitle.str().c_str());
#endif
}

void Framework::Audio_Initialise()
{
	LogInfo("Initialise Audio");

	for (auto &soundBackendName : Settings->getString("Audio.Backends").split(':')) {
		auto backendFactory = registeredSoundBackends->find(soundBackendName);
		if (backendFactory == registeredSoundBackends->end()) {
			LogInfo("Sound backend %s not in supported list", soundBackendName.str().c_str());
			continue;
		}
		SoundBackend *backend = backendFactory->second->create();
		if (!backend) {
			LogInfo("Sound backend %s failed to init", soundBackendName.str().c_str());
			continue;
		}
		this->soundBackend.reset(backend);
		LogInfo("Using sound backend %s", soundBackendName.str().c_str());
		break;
	}
	if (!this->soundBackend) {
		LogError("No functional sound backend found");
	}
	this->jukebox.reset(new JukeBoxImpl(*this));

	/* Setup initial gain */
	this->soundBackend->setGain(SoundBackend::Gain::Global,
	                            (float)this->Settings->getInt("Audio.GlobalGain") / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Music,
	                            (float)this->Settings->getInt("Audio.MusicGain") / 20.0f);
	this->soundBackend->setGain(SoundBackend::Gain::Sample,
	                            (float)this->Settings->getInt("Audio.SampleGain") / 20.0f);
}

void Framework::Audio_Shutdown()
{
	LogInfo("Shutdown Audio");
	this->jukebox.reset();
	this->soundBackend.reset();
}

std::shared_ptr<Stage> Framework::Stage_GetPrevious() { return p->ProgramStages.Previous(); }

std::shared_ptr<Stage> Framework::Stage_GetPrevious(std::shared_ptr<Stage> From)
{
	return p->ProgramStages.Previous(From);
}

} // namespace OpenApoc
