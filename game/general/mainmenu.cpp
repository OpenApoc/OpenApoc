
#include "mainmenu.h"
#include "../../framework/framework.h"

void MainMenu::Begin()
{
	ufopediaimg = al_load_bitmap( "data/UFODATA/B-SETUP.PCX" );
	fontpalette = new Palette( "UFODATA/PAL_01.DAT" );
	largefont = new ApocalypseFont( true, fontpalette );
	smallfont = new ApocalypseFont( false, fontpalette );
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
	std::string* s;
	al_draw_bitmap( ufopediaimg, 0, 0, 0 );
	largefont->DrawString( FRAMEWORK->Display_GetWidth() / 2, 10, "OPEN/APOC", APOCFONT_ALIGN_CENTRE );
	s = currentlanguage->GetText( "STR_START_CAMPAIGN" );
	smallfont->DrawString( 320, 180, *s, APOCFONT_ALIGN_CENTRE );
	s = currentlanguage->GetText( "STR_LOAD_SAVED_GAME" );
	smallfont->DrawString( 320, 220, *s, APOCFONT_ALIGN_CENTRE );
	s = currentlanguage->GetText( "STR_QUIT" );
	smallfont->DrawString( 320, 260, *s, APOCFONT_ALIGN_CENTRE );
}

bool MainMenu::IsTransition()
{
	return false;
}
