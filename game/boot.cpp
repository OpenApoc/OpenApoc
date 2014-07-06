
#include "boot.h"
#include "../framework/framework.h"
#include "general/mainmenu.h"

void BootUp::Begin()
{
	FRAMEWORK->Display_SetTitle( new std::string("OpenApocalypse") );
	ufopediaimg = al_load_bitmap( "data/UFODATA/TITLES.PCX" );
	loadtime = 0;
}

void BootUp::Pause()
{
}

void BootUp::Resume()
{
}

void BootUp::Finish()
{
	al_destroy_bitmap( ufopediaimg );
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
	if( loadtime >= FRAMES_PER_SECOND )
	{
		StartGame();
	}
}

void BootUp::Render()
{
	al_draw_bitmap( ufopediaimg, 0, 0, 0 );
}

void BootUp::StartGame()
{
	delete FRAMEWORK->ProgramStages->Pop();
	FRAMEWORK->ProgramStages->Push( new MainMenu() );
}

bool BootUp::IsTransition()
{
	return false;
}
