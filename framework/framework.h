
#pragma once

//windows workaround
#ifndef __func__
#define __func__ __FUNCTION__
#endif

#include "logger.h"
#include "includes.h"
#include "event.h"
#include "data.h"
#include "stagestack.h"
#include "renderer.h"
#include "sound.h"

#include "library/configfile.h"

#include "game/gamestate.h"

//FIXME: Remove core-allegro
//Required for input types
#include <allegro5/allegro.h>

namespace OpenApoc {

class Shader;
class GameCore;

#define FRAMES_PER_SECOND 100

class FrameworkPrivate;

class Framework
{
	private:
		std::unique_ptr<FrameworkPrivate> p;
		std::string programName;
		void Audio_Initialise();
		void Audio_Shutdown();
	public:
		std::unique_ptr<Data> data;
		GameState state;
		std::unique_ptr<GameCore> gamecore;

		std::unique_ptr<ConfigFile> Settings;
		std::unique_ptr<Renderer> renderer;
		std::unique_ptr<SoundBackend> soundBackend;
		std::unique_ptr<JukeBox> jukebox;

		Framework(const std::string programName, const std::vector<std::string> cmdline);
		~Framework();

		void Run();
		void ProcessEvents();
		void PushEvent( Event* e );
		void TranslateAllegroEvents();
		void ShutdownFramework();
		bool IsShuttingDown();

		void SaveSettings();

		void Display_Initialise();
		void Display_Shutdown();
		int Display_GetWidth();
		int Display_GetHeight();
		void Display_SetTitle( std::string* NewTitle );
		void Display_SetTitle( std::string NewTitle );

		bool IsSlowMode();
		void SetSlowMode(bool SlowEnabled);
};

}; //namespace OpenApoc
