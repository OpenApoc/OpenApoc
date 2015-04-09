
#pragma once

#include "includes.h"
#include "event.h"
#include "data.h"
#include "stagestack.h"
#include "renderer.h"

#include "library/configfile.h"

#include "game/gamestate.h"

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
	public:
		std::unique_ptr<Data> data;
		GameState state;
		std::unique_ptr<GameCore> gamecore;

		std::unique_ptr<ConfigFile> Settings;
		std::unique_ptr<Renderer> renderer;

		Framework(const std::string programName);
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

		void Audio_Initialise();
		void Audio_Shutdown();
		void Audio_PlayAudio( std::string Filename, bool Loop );
		void Audio_StopAudio();

		bool IsSlowMode();
		void SetSlowMode(bool SlowEnabled);
};

}; //namespace OpenApoc
