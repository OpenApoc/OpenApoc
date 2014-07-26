
#include "mainmenu.h"
#include "../../framework/framework.h"

MainMenu::MainMenu()
{
	mousecursor = new Cursor( GAMECORE->GetPalette( "TACDATA/TACTICAL.PAL" ) );

	testform = GAMECORE->GetForm("FORM_MAINMENU");
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
}

bool MainMenu::IsTransition()
{
	return false;
}
