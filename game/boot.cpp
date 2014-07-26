
#include "boot.h"
#include "../framework/framework.h"
#include "general/mainmenu.h"
#include "../transitions/transitions.h"
#include "resources/gamecore.h"

void BootUp::Begin()
{
	loadtime = 0;
	FRAMEWORK->Display_SetTitle("OpenApocalypse");

	threadload = nullptr;
	//threadload = al_create_thread( CreateGameCore, nullptr );
	if( threadload != nullptr )
	{
		al_start_thread( threadload );
	}
}

void BootUp::Pause()
{
}

void BootUp::Resume()
{
}

void BootUp::Finish()
{
}

void BootUp::EventOccurred(Event *e)
{
	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			delete FRAMEWORK->ProgramStages->Pop();
		} else {
			loadtime = FRAMES_PER_SECOND;
		}
	}
}

void BootUp::Update()
{
	if( threadload == nullptr && GAMECORE == nullptr )
	{
		CreateGameCore( nullptr, nullptr );
	}

	if( GAMECORE != nullptr && GAMECORE->Loaded )
	{
		StartGame();
	}
}

void BootUp::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
}

void BootUp::StartGame()
{
	delete FRAMEWORK->ProgramStages->Pop();
	FRAMEWORK->ProgramStages->Push( new TransitionFadeIn( new MainMenu(), al_map_rgb( 0, 0, 0 ), FRAMES_PER_SECOND ) );
}

bool BootUp::IsTransition()
{
	return false;
}

void* BootUp::CreateGameCore(ALLEGRO_THREAD* thread, void* args)
{
	std::string ruleset(*FRAMEWORK->Settings->GetQuickStringValue( "GameRules", "XCOMAPOC.XML" ));
	std::string language(*FRAMEWORK->Settings->GetQuickStringValue( "Language", "en_gb" ));

	GameCore* c = new GameCore( ruleset, language );

	if( thread != nullptr )
	{
		al_destroy_thread( thread );
	}

	return nullptr;
}
