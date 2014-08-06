
#include "mainmenu.h"
#include "../../framework/framework.h"

MainMenu::MainMenu()
{
	mousecursor = new Cursor( GAMECORE->GetPalette( "TACDATA/TACTICAL.PAL" ) );

	// testform = GAMECORE->GetForm("FORM_UFOPAEDIA_TITLE");
	testform = GAMECORE->GetForm("FORM_MAINMENU");

	testpck = new PCK( "TACDATA/ICON_M.PCK", "TACDATA/ICON_M.TAB", GAMECORE->GetPalette( "TACDATA/TACTICAL.PAL" ) );
}

MainMenu::~MainMenu()
{
	delete mousecursor;
}

void MainMenu::Begin()
{
	musicplayer = new Music( 26 );
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
	delete musicplayer;
}

void MainMenu::EventOccurred(Event *e)
{
	bool washandled = false;
	testform->EventOccured( e );
	mousecursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT" )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}
}

void MainMenu::Update()
{
	testform->Update();
}

void MainMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	testform->Render();
	mousecursor->Render();

	int x = 0;
	int y = 0;

	for( int c = 0; c < testpck->GetImageCount(); c++ )
	{
		testpck->RenderImage( c, x, y );
		x += 80;
		if( x > FRAMEWORK->Display_GetWidth() )
		{
			x = 0;
			y += 60;
		}
	}

}

bool MainMenu::IsTransition()
{
	return false;
}
