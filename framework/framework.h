
#pragma once

#include "includes.h"
#include "event.h"

#include "../library/configfile.h"
#include "stagestack.h"

#ifndef Shader
class Shader;
#endif

#define FRAMES_PER_SECOND 100

#define FRAMEWORK	Framework::System

class Framework
{
  private:
    bool quitProgram;

		ALLEGRO_TIMER* frameTimer;
    int framesToProcess;

    ALLEGRO_DISPLAY_MODE screenMode;
		ALLEGRO_DISPLAY* screen;
		ALLEGRO_BITMAP* screenRetarget;

		ALLEGRO_EVENT_QUEUE* eventAllegro;
		std::list<Event*> eventQueue;
		ALLEGRO_MUTEX* eventMutex;

		ALLEGRO_MIXER* audioMixer;
		ALLEGRO_VOICE* audioVoice;

		Shader* activeShader;

  public:
    static Framework* System;

    ConfigFile* Settings;
    StageStack* ProgramStages;

    Framework();
    ~Framework();

    void Run();
		void ProcessEvents();
    void PushEvent( Event* e );
		void TranslateAllegroEvents();
		void ShutdownFramework();
		bool IsShuttingDown() { return quitProgram; };

    void SaveSettings();

    void Display_Initialise();
    void Display_Shutdown();
    int Display_GetWidth();
    int Display_GetHeight();
		void Display_SetTitle( std::string* NewTitle );
		void Display_SetTitle( std::string NewTitle );
		ALLEGRO_BITMAP* Display_GetCurrentTarget();
		void Display_SetTarget();
		void Display_SetTarget( ALLEGRO_BITMAP* Target );
		void Display_SetShader();
		void Display_SetShader( Shader* NewShader );

    void Audio_Initialise();
    void Audio_Shutdown();
    void Audio_PlayAudio( std::string Filename, bool Loop );
    void Audio_StopAudio();
		ALLEGRO_MIXER* Audio_GetMixer();

};
