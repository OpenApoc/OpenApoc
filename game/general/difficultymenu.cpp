#include "framework/framework.h"
#include "game/general/difficultymenu.h"
#include "game/city/city.h"
#include "game/city/cityview.h"

namespace OpenApoc
{

DifficultyMenu::DifficultyMenu()
    : Stage(), difficultymenuform(fw().gamecore->GetForm("FORM_DIFFICULTYMENU"))
{
	assert(difficultymenuform);
}

DifficultyMenu::~DifficultyMenu() {}

void DifficultyMenu::Begin() {}

void DifficultyMenu::Pause() {}

void DifficultyMenu::Resume() {}

void DifficultyMenu::Finish() {}

void DifficultyMenu::EventOccurred(Event *e)
{
	difficultymenuform->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		UString ruleName;
		if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY1") == 0)
		{
			ruleName = "rules/difficulty1.xml";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY2") == 0)
		{
			ruleName = "rules/difficulty2.xml";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY3") == 0)
		{
			ruleName = "rules/difficulty3.xml";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY4") == 0)
		{
			ruleName = "rules/difficulty4.xml";
		}
		else if (e->Forms().RaisedBy->Name.compare("BUTTON_DIFFICULTY5") == 0)
		{
			ruleName = "rules/difficulty5.xml";
		}
		else
		{
			LogWarning("Unknown button pressed: %s", e->Forms().RaisedBy->Name.c_str());
			return;
		}

		auto state = std::make_shared<GameState>(ruleName);

		stageCmd.cmd = StageCmd::Command::REPLACE;
		stageCmd.nextStage = std::make_shared<CityView>(state);
		return;
	}
}

void DifficultyMenu::Update(StageCmd *const cmd)
{
	difficultymenuform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void DifficultyMenu::Render()
{
	difficultymenuform->Render();
	fw().gamecore->MouseCursor->Render();
}

bool DifficultyMenu::IsTransition() { return false; }
}; // namespace OpenApoc
