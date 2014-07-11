
#include "boot.h"
#include "../framework/framework.h"
#include "general/mainmenu.h"
#include "../transitions/transitions.h"

void BootUp::Begin()
{
	loadtime = 0;
	FRAMEWORK->Display_SetTitle("OpenApocalypse");
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
	loadtime++;
	if( loadtime >= FRAMES_PER_SECOND / 2 )
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
