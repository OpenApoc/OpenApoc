
#include "mainmenu.h"
#include "../../framework/framework.h"

void MainMenu::Begin()
{
	ufopediaimg = al_load_bitmap( "data/UFODATA/B-SETUP.PCX" );
	currentlanguage = new Language( "EN-GB" );
	buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
}

void MainMenu::Pause()
{
}

void MainMenu::Resume()
{
}

void MainMenu::Finish()
{
	al_destroy_bitmap( ufopediaimg );
	delete currentlanguage;
	delete buttonclick;
}

void MainMenu::EventOccurred(Event *e)
{
	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			delete FRAMEWORK->ProgramStages->Pop();
		}
	}

	if( e->Type == EVENT_MOUSE_DOWN )
	{
		buttonclick->PlaySound();
	}
}

void MainMenu::Update()
{
}

void MainMenu::Render()
{
	al_draw_bitmap( ufopediaimg, 0, 0, 0 );
}

bool MainMenu::IsTransition()
{
	return false;
}
