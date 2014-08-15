
#include "mainmenu.h"
#include "../../framework/framework.h"
#include "optionsmenu.h"
#include "../../transitions/transitions.h"

MainMenu::MainMenu()
{
	mainmenuform = GAMECORE->GetForm("FORM_MAINMENU");

	TextEdit* e = new TextEdit(mainmenuform, "Testing", new ApocalypseFont( ApocalypseFont::SmallFont, GAMECORE->GetPalette("UI/UI_PALETTE.PNG") ));
	e->Location.X = 20;
	e->Location.Y = 60;
	e->Size.X = 200;
	e->Size.Y = 32;
	e->SetText("Dummy");
	e->TextVAlign = VerticalAlignment::Centre;
}

MainMenu::~MainMenu()
{
}

void MainMenu::Begin()
{
	musicplayer = new Music( 0 );
	musicplayer->Play();
}

void MainMenu::Pause()
{
	musicplayer->Stop();
}

void MainMenu::Resume()
{
	musicplayer->Play();
}

void MainMenu::Finish()
{
	delete musicplayer;
}

void MainMenu::EventOccurred(Event *e)
{
	mainmenuform->EventOccured( e );
	GAMECORE->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_OPTIONS" )
		{
			FRAMEWORK->ProgramStages->Push( new TransitionFadeAcross( new OptionsMenu(), FRAMES_PER_SECOND >> 2 ) );
			return;
		}
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT" )
		{
			FRAMEWORK->ShutdownFramework();
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::CheckBoxChange )
	{
		if( e->Data.Forms.RaisedBy->Name == "CHECK_DEBUGMODE" )
		{
			GAMECORE->DebugModeEnabled = ((CheckBox*)e->Data.Forms.RaisedBy)->Checked;
		}
	}

}

void MainMenu::Update()
{
	mainmenuform->Update();
}

void MainMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	mainmenuform->Render();
	GAMECORE->MouseCursor->Render();
}

bool MainMenu::IsTransition()
{
	return false;
}
