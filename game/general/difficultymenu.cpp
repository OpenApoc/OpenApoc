
#include "framework/framework.h"
#include "game/general/difficultymenu.h"
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
		UString citymapName;
		if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			citymapName = U8Str(u8"CITYMAP1");
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			citymapName = U8Str(u8"CITYMAP2");
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			citymapName = U8Str(u8"CITYMAP3");
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			citymapName = U8Str(u8"CITYMAP4");
		}
		else if (e->Data.Forms.RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			citymapName = U8Str(u8"CITYMAP5");
		}
		else
		{
			LogWarning("Unknown button pressed: %S", e->Data.Forms.RaisedBy->Name.getTerminatedBuffer());
			citymapName = U8Str(u8"CITYMAP1");
			return;
		}
		fw.state.city.reset(new City(fw, citymapName));
		stageCmd.cmd = StageCmd::Command::REPLACE;
		stageCmd.nextStage = std::make_shared<TileView>(fw, *fw.state.city, Vec3<int>{CITY_TILE_X, CITY_TILE_Y, CITY_TILE_Z});
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
	difficultymenuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool DifficultyMenu::IsTransition()
{
	return false;
}
}; //namespace OpenApoc
