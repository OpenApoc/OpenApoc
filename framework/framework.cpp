
#include "framework.h"
#include "game/boot.h"
#include "game/resources/gamecore.h"
#include "game/city/city.h"
#include "renderer.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

namespace {

static std::map<std::string, std::string> defaultConfig =
{
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
};

};

namespace OpenApoc {

class FrameworkPrivate
{
	private:
		friend class Framework;
		bool quitProgram;

		ALLEGRO_TIMER *frameTimer;
		int framesToProcess;
		bool enableSlowDown;

		ALLEGRO_DISPLAY_MODE screenMode;
		ALLEGRO_DISPLAY *screen;

		ALLEGRO_EVENT_QUEUE *eventAllegro;
		std::list<Event*> eventQueue;
		ALLEGRO_MUTEX *eventMutex;

		ALLEGRO_MIXER *audioMixer;
		ALLEGRO_VOICE *audioVoice;

		StageStack ProgramStages;
		std::shared_ptr<Surface> defaultSurface;
};



Framework::Framework(const std::string dataRoot)
	: data(dataRoot), p(new FrameworkPrivate)
{
#ifdef WRITE_LOG
	printf( "Framework: Startup: Allegro\n" );
#endif

	if( !al_init() )
	{
		printf( "Framework: Error: Cannot init Allegro\n" );
		p->quitProgram = true;
		return;
	}

	al_init_font_addon();
	if( !al_install_keyboard() || !al_install_mouse() || !al_init_ttf_addon())
	{
		printf( "Framework: Error: Cannot init Allegro plugin\n" );
		p->quitProgram = true;
		return;
	}

#ifdef NETWORK_SUPPORT
	if( enet_initialize() != 0 )
	{
		printf( "Framework: Error: Cannot init enet\n" );
		p->quitProgram = true;
		return;
	}
#endif

#ifdef WRITE_LOG
	printf( "Framework: Startup: Variables and Config\n" );
#endif
	p->quitProgram = false;
	p->framesToProcess = 0;
	Settings.reset(new ConfigFile( "settings.cfg", defaultConfig));

	p->eventAllegro = al_create_event_queue();
	p->eventMutex = al_create_mutex_recursive();
	p->frameTimer = al_create_timer( 1.0 / FRAMES_PER_SECOND );

	srand( (unsigned int)al_get_time() );

	Display_Initialise();
	Audio_Initialise();

	al_register_event_source( p->eventAllegro, al_get_display_event_source( p->screen ) );
	al_register_event_source( p->eventAllegro, al_get_keyboard_event_source() );
	al_register_event_source( p->eventAllegro, al_get_mouse_event_source() );
	al_register_event_source( p->eventAllegro, al_get_timer_event_source( p->frameTimer ) );

#ifdef FRAMEWORK_SPEED_SLOWDOWN
	p->enableSlowDown = true;
#else
	p->enableSlowDown = false;
#endif

}

Framework::~Framework()
{
	//Kill gamecore and program stages first, so any resources are cleaned before
	//allegro is de-inited
	state.clear();
	gamecore.reset();
	p->ProgramStages.Clear();
#ifdef WRITE_LOG
	printf( "Framework: Save Config\n" );
#endif
	SaveSettings();

#ifdef WRITE_LOG
	printf( "Framework: Shutdown\n" );
#endif
	Display_Shutdown();
	Audio_Shutdown();
	al_destroy_event_queue( p->eventAllegro );
	al_destroy_mutex( p->eventMutex );
	al_destroy_timer( p->frameTimer );
	
#ifdef NETWORK_SUPPORT
#ifdef WRITE_LOG
	printf( "Framework: Shutdown enet\n" );
#endif
	enet_deinitialize();
#endif

#ifdef WRITE_LOG
	printf( "Framework: Shutdown Allegro\n" );
#endif
	al_shutdown_ttf_addon();
	al_uninstall_mouse();
	al_uninstall_keyboard();
	al_shutdown_font_addon();

	al_uninstall_system();
}

void Framework::Run()
{
#ifdef WRITE_LOG
	printf( "Framework: Run.Program Loop\n" );
#endif

	p->ProgramStages.Push( std::make_shared<BootUp>(*this) );

	al_start_timer( p->frameTimer );

	while( !p->quitProgram )
	{
		RendererSurfaceBinding b(*this->renderer, p->defaultSurface);
		this->renderer->clear();
		ProcessEvents();
		while( p->framesToProcess > 0 )
		{
			StageCmd cmd;
			if( p->ProgramStages.IsEmpty() )
			{
				break;
			}
			p->ProgramStages.Current()->Update(&cmd);
			switch (cmd.cmd)
			{
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
			p->framesToProcess--;
		}
		if( !p->ProgramStages.IsEmpty() )
		{
			p->ProgramStages.Current()->Render();
			al_flip_display();
		}
	}
}

void Framework::ProcessEvents()
{
#ifdef WRITE_LOG
	printf( "Framework: ProcessEvents\n" );
#endif

	if( p->ProgramStages.IsEmpty() )
	{
		p->quitProgram = true;
		return;
	}

	// Convert Allegro events before we process
	// TODO: Consider threading the translation
	TranslateAllegroEvents();

	al_lock_mutex( p->eventMutex );

	while( p->eventQueue.size() > 0 && !p->ProgramStages.IsEmpty() )
	{
		Event* e;
		e = p->eventQueue.front();
		p->eventQueue.pop_front();
		switch( e->Type )
		{
			case EVENT_WINDOW_CLOSED:
				delete e;
				al_unlock_mutex( p->eventMutex );
				ShutdownFramework();
				return;
				break;
			default:
				p->ProgramStages.Current()->EventOccurred( e );
				break;
		}
		delete e;
	}

	al_unlock_mutex( p->eventMutex );
}

void Framework::PushEvent( Event* e )
{
	al_lock_mutex( p->eventMutex );
	p->eventQueue.push_back( e );
	al_unlock_mutex( p->eventMutex );
}

void Framework::TranslateAllegroEvents()
{
	ALLEGRO_EVENT e;
	Event* fwE;

	while( al_get_next_event( p->eventAllegro, &e ) )
	{
		switch( e.type )
		{
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
				fwE = new Event();
				fwE->Type = EVENT_WINDOW_CLOSED;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_JOYSTICK_CONFIGURATION:
				al_reconfigure_joysticks();
				break;
			case ALLEGRO_EVENT_TIMER:
				if( e.timer.source == p->frameTimer )
				{
					if( p->enableSlowDown )
					{
						// Slow the game down, never process more than one update per frame
						p->framesToProcess = 1;
					} else {
						p->framesToProcess++;
					}
				} else {
					fwE = new Event();
					fwE->Type = EVENT_TIMER_TICK;
					fwE->Data.Timer.TimerObject = (void*)e.timer.source;
					PushEvent( fwE );
				}
				break;
			case ALLEGRO_EVENT_KEY_DOWN:
				fwE = new Event();
				fwE->Type = EVENT_KEY_DOWN;
				fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
				fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
				fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_KEY_UP:
				fwE = new Event();
				fwE->Type = EVENT_KEY_UP;
				fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
				fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
				fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_KEY_CHAR:
				fwE = new Event();
				fwE->Type = EVENT_KEY_PRESS;
				fwE->Data.Keyboard.KeyCode = e.keyboard.keycode;
				fwE->Data.Keyboard.UniChar = e.keyboard.unichar;
				fwE->Data.Keyboard.Modifiers = e.keyboard.modifiers;
				PushEvent( fwE );
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
				PushEvent( fwE );
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
				PushEvent( fwE );
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
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
				fwE = new Event();
				fwE->Type = EVENT_WINDOW_RESIZE;
				fwE->Data.Display.X = 0;
				fwE->Data.Display.Y = 0;
				fwE->Data.Display.Width = al_get_display_width( p->screen );
				fwE->Data.Display.Height = al_get_display_height( p->screen );
				fwE->Data.Display.Active = true;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
				fwE = new Event();
				fwE->Type = EVENT_WINDOW_ACTIVATE;
				fwE->Data.Display.X = 0;
				fwE->Data.Display.Y = 0;
				fwE->Data.Display.Width = al_get_display_width( p->screen );
				fwE->Data.Display.Height = al_get_display_height( p->screen );
				fwE->Data.Display.Active = true;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
				fwE = new Event();
				fwE->Type = EVENT_WINDOW_DEACTIVATE;
				fwE->Data.Display.X = 0;
				fwE->Data.Display.Y = 0;
				fwE->Data.Display.Width = al_get_display_width( p->screen );
				fwE->Data.Display.Height = al_get_display_height( p->screen );
				fwE->Data.Display.Active = false;
				PushEvent( fwE );
				break;
			case ALLEGRO_EVENT_AUDIO_STREAM_FINISHED:
				fwE = new Event();
				fwE->Type = EVENT_AUDIO_STREAM_FINISHED;
				PushEvent( fwE );
				break;
			default:
				fwE = new Event();
				fwE->Type = EVENT_UNDEFINED;
				PushEvent( fwE );
				break;
		}
	}
}

void Framework::ShutdownFramework()
{
#ifdef WRITE_LOG
	printf( "Framework: Shutdown Framework\n" );
#endif
	p->ProgramStages.Clear();
	p->quitProgram = true;
}

void Framework::SaveSettings()
{
	// Just to keep the filename consistant
	Settings->save( "settings.cfg" );
}

void Framework::Display_Initialise()
{
#ifdef WRITE_LOG
	printf( "Framework: Initialise Display\n" );
#endif
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

	if( scrFS )
	{
		display_flags |= ALLEGRO_FULLSCREEN;
	}

	al_set_new_display_flags(display_flags);

	p->screen = al_create_display( scrW, scrH );

	if (!p->screen)
	{
		std::cerr << "Failed to create screen\n";
		exit(1);
	}

	al_set_blender( ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA );

	al_hide_mouse_cursor( p->screen );

	this->renderer.reset(Renderer::createRenderer());
	this->p->defaultSurface.reset(this->renderer->getDefaultSurface());

	std::cout << "Using renderer: " << this->renderer->getName() << "\n";

}

void Framework::Display_Shutdown()
{
#ifdef WRITE_LOG
	printf( "Framework: Shutdown Display\n" );
#endif

	al_unregister_event_source( p->eventAllegro, al_get_display_event_source( p->screen ) );
	al_destroy_display( p->screen );
}

int Framework::Display_GetWidth()
{
	return al_get_display_width( p->screen );
}

int Framework::Display_GetHeight()
{
	return al_get_display_height( p->screen );
}

void Framework::Display_SetTitle( std::string* NewTitle )
{
	std::wstring widestr = std::wstring(NewTitle->begin(), NewTitle->end());
	al_set_app_name( (char*)widestr.c_str() );
	al_set_window_title( p->screen, (char*)widestr.c_str() );
}

void Framework::Display_SetTitle( std::string NewTitle )
{
	std::wstring widestr = std::wstring(NewTitle.begin(), NewTitle.end());
	al_set_app_name( (char*)widestr.c_str() );
	al_set_window_title( p->screen, (char*)widestr.c_str() );
}

void Framework::Audio_Initialise()
{
#ifdef WRITE_LOG
	printf( "Framework: Initialise Audio\n" );
#endif

	p->audioVoice = 0;
	p->audioMixer = 0;

	if( !al_install_audio() )
	{
		printf( "Audio_Initialise: Failed to install audio\n" );
		return;
	}
	if( !al_init_acodec_addon() )
	{
		printf( "Audio_Initialise: Failed to install codecs\n" );
		return;
	}

	// Allow playing samples
	al_reserve_samples( 10 );

	p->audioVoice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16, ALLEGRO_CHANNEL_CONF_2);
	if( p->audioVoice == 0 )
	{
		printf( "Audio_Initialise: Failed to create voice\n" );
		return;
	}
	p->audioMixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32, ALLEGRO_CHANNEL_CONF_2);
	if( p->audioMixer == 0 )
	{
		printf( "Audio_Initialise: Failed to create mixer\n" );
		al_destroy_voice( p->audioVoice );
		p->audioVoice = 0;
		return;
	}
	if( !al_attach_mixer_to_voice( p->audioMixer, p->audioVoice ) )
	{
		printf( "Audio_Initialise: Failed to attach mixer to voice\n" );
		al_destroy_voice( p->audioVoice );
		p->audioVoice = 0;
		al_destroy_mixer( p->audioMixer );
		p->audioMixer = 0;
		return;
	}
}

void Framework::Audio_Shutdown()
{
#ifdef WRITE_LOG
	printf( "Framework: Shutdown Audio\n" );
#endif
	if( p->audioVoice != 0 )
	{
		al_destroy_voice( p->audioVoice );
		p->audioVoice = 0;
	}
	if( p->audioMixer != 0 )
	{
		al_destroy_mixer( p->audioMixer );
		p->audioMixer = 0;
	}
	al_uninstall_audio();
}

void Framework::Audio_PlayAudio( std::string Filename, bool Loop )
{
	if( p->audioVoice == 0 || p->audioMixer == 0 )
	{
		return;
	}

#ifdef WRITE_LOG
	printf( "Framework: Start audio file %s\n", Filename.c_str() );
#endif


}

void Framework::Audio_StopAudio()
{
	if( p->audioVoice == 0 || p->audioMixer == 0 )
	{
		return;
	}

#ifdef WRITE_LOG
	printf( "Framework: Stop audio\n" );
#endif
}

bool Framework::IsSlowMode()
{
	return p->enableSlowDown;
}

void Framework::SetSlowMode(bool SlowEnabled)
{
	p->enableSlowDown = SlowEnabled;
}

}; //namespace OpenApoc
