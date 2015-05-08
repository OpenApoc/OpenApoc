
#include "framework/framework.h"
#include "difficultymenu.h"
#include "game/city/city.h"
#include "game/tileview/tileview.h"

namespace OpenApoc {

DifficultyMenu::DifficultyMenu(Framework &fw)
	: Stage(fw)
{
	difficultymenuform = fw.gamecore->GetForm("FORM_DIFFICULTYMENU");
	assert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu()
{
}

void DifficultyMenu::Begin()
{
}

void DifficultyMenu::Pause()
{
}

void DifficultyMenu::Resume()
{
}

void DifficultyMenu::Finish()
{
}

void DifficultyMenu::EventOccurred(Event *e)
{
	difficultymenuform->EventOccured( e );
	fw.gamecore->MouseCursor->EventOccured( e );

	if( e->Type == EVENT_KEY_DOWN )
	{
		if( e->Data.Keyboard.KeyCode == ALLEGRO_KEY_ESCAPE )
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if( e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick )
	{
		std::string citymapName;
		if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			citymapName = "CITYMAP1";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			citymapName = "CITYMAP2";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			citymapName = "CITYMAP3";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			citymapName = "CITYMAP4";
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			citymapName = "CITYMAP5";
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->Data.Forms.RaisedBy->Name.c_str());
			citymapName = "CITYMAP1";
			return;
		}
		fw.state.city.reset(new City(fw, citymapName));
		stageCmd.cmd = StageCmd::Command::REPLACE;
		stageCmd.nextStage = std::make_shared<TileView>(fw, *fw.state.city, Vec3<float>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z});
		return;
	}
}

void DifficultyMenu::Update(StageCmd * const cmd)
{
	difficultymenuform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void DifficultyMenu::Render()
{
	al_clear_to_color( al_map_rgb( 0, 0, 0 ) );
	difficultymenuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool DifficultyMenu::IsTransition()
{
	return false;
}
}; //namespace OpenApoc
