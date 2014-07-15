
#include "mainmenu.h"
#include "../../framework/framework.h"

MainMenu::MainMenu()
{
	emptybackground = al_load_bitmap( "data/UFODATA/TITLES.PCX" );

	ALLEGRO_BITMAP* titlesbackground = al_load_bitmap( "data/UFODATA/B-SETUP.PCX" );
	buttonimage = al_create_bitmap( 262, 29 );
	FRAMEWORK->Display_SetTarget( buttonimage );
	al_clear_to_color( al_map_rgba( 0, 0, 0, 0 ) );
	al_draw_filled_rectangle( 3, 2, 262, 29, al_map_rgba( 0, 0, 0, 192 ) );
	al_draw_bitmap_region( titlesbackground, 188, 173, 259, 27, 0, 0, 0 );
	FRAMEWORK->Display_SetTarget();
	al_destroy_bitmap( titlesbackground );

	fontpalette = new Palette( "UFODATA/PAL_02.DAT" );
	//fontpalette = new Palette( "TACDATA/TACTICAL.PAL" );
	largefont = new ApocalypseFont( ApocalypseFont::LargeFont, new Palette( "UFODATA/PAL_06.DAT" ) );
	smallfont = new ApocalypseFont( ApocalypseFont::SmallFont, fontpalette );
	currentlanguage = new Language( "EN-GB" );
	buttonclick = new RawSound( "STRATEGC/INTRFACE/BUTTON1.RAW" );
	musicplayer = new Music( 25 );
	mousecursor = new Cursor( fontpalette );

	testpck = new PCK( "data/UFODATA/VEHICLE.PCK", "data/UFODATA/VEHICLE.TAB", false, fontpalette );
	//testpck = new PCK( "data/MAPS/05RESCUE/MAPUNITS/FEATURE.PCK", "data/MAPS/05RESCUE/MAPUNITS/FEATURE.TAB", false, fontpalette );
}

MainMenu::~MainMenu()
{
	al_destroy_bitmap( emptybackground );
	al_destroy_bitmap( buttonimage );
	delete largefont;
	delete smallfont;
	delete currentlanguage;
	delete buttonclick;
	delete musicplayer;
	delete mousecursor;
}

void MainMenu::Begin()
{
	musicplayer->Play();
}

void MainMenu::Pause()
{
}

void MainMenu::Resume()
{
}

void MainMenu::Finish()
{
}

void MainMenu::EventOccurred(Event *e)
{
	mousecursor->EventOccured(e);
	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			delete FRAMEWORK->ProgramStages->Pop();
		} else {
			musicplayer->Play();
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
	al_draw_bitmap( emptybackground, 0, 0, 0 );

	al_draw_filled_rectangle( 0, 0, 640, 32, al_map_rgba( 255, 255, 255, 128 ) );
	largefont->DrawString( 320, 4, "OPEN APOCALYPSE", APOCFONT_ALIGN_CENTRE );

	al_draw_bitmap( buttonimage, 188, 176, 0 );
	s = currentlanguage->GetText( "STR_START_CAMPAIGN" );
	smallfont->DrawString( 320, 180, *s, APOCFONT_ALIGN_CENTRE );

	al_draw_bitmap( buttonimage, 188, 208, 0 );
	s = currentlanguage->GetText( "STR_LOAD_SAVED_GAME" );
	smallfont->DrawString( 320, 212, *s, APOCFONT_ALIGN_CENTRE );

	al_draw_bitmap( buttonimage, 188, 240, 0 );
	s = currentlanguage->GetText( "STR_OPTIONS" );
	smallfont->DrawString( 320, 244, *s, APOCFONT_ALIGN_CENTRE );

	al_draw_bitmap( buttonimage, 188, 272, 0 );
	s = currentlanguage->GetText( "STR_QUIT" );
	smallfont->DrawString( 320, 276, *s, APOCFONT_ALIGN_CENTRE );

	al_draw_bitmap( buttonimage, 188, 304, 0 );
	s = currentlanguage->GetText( "STR_CHEATING" );
	smallfont->DrawString( 320, 308, *s, APOCFONT_ALIGN_CENTRE );

	mousecursor->Render();

	for( int x = 0; x < testpck->GetImageCount(); x++ )
	{
		testpck->RenderImage( x, (x * 40) % 640, ((x * 40) / 640) * 40 );
	}
}

bool MainMenu::IsTransition()
{
	return false;
}
