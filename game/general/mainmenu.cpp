
#include "mainmenu.h"
#include "framework/framework.h"
#include "optionsmenu.h"
#include "difficultymenu.h"

namespace OpenApoc {

std::vector<std::string> tracks {"music:0"};

MainMenu::MainMenu(Framework &fw)
	: Stage(fw)
{
	mainmenuform = fw.gamecore->GetForm("FORM_MAINMENU");
}

MainMenu::~MainMenu()
{
}

void MainMenu::Begin()
{
	fw.jukebox->play(tracks);
}

void MainMenu::Pause()
{
	fw.jukebox->stop();
}

void MainMenu::Resume()
{
	fw.jukebox->play(tracks);
}

void MainMenu::Finish()
{
}

void MainMenu::EventOccurred(Event *e)
{
	mainmenuform->EventOccured( e );
	fw.gamecore->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_OPTIONS" )
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<OptionsMenu>(fw);
			return;
		}
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT" )
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
		if( e->Data.Forms.RaisedBy->Name == "BUTTON_NEWGAME" )
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = std::make_shared<DifficultyMenu>(fw);
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::CheckBoxChange )
	{
		if( e->Data.Forms.RaisedBy->Name == "CHECK_DEBUGMODE" )
		{
			fw.gamecore->DebugModeEnabled = ((CheckBox*)e->Data.Forms.RaisedBy)->Checked;
		}
	}

}

void MainMenu::Update(StageCmd * const cmd)
{
	mainmenuform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void MainMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	mainmenuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool MainMenu::IsTransition()
{
	return false;
}
}; //namespace OpenApoc
