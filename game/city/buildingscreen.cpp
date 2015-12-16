
#include "game/city/buildingscreen.h"
#include "framework/framework.h"

namespace OpenApoc
{

BuildingScreen::BuildingScreen(Framework &fw)
    : Stage(fw), menuform(fw.gamecore->GetForm("FORM_BUILDING_SCREEN"))
{
}

BuildingScreen::~BuildingScreen() {}

void BuildingScreen::Begin() {}

void BuildingScreen::Pause() {}

void BuildingScreen::Resume() {}

void BuildingScreen::Finish() {}

void BuildingScreen::EventOccurred(Event *e)
{
	menuform->EventOccured(e);
	fw.gamecore->MouseCursor->EventOccured(e);

	if (e->Type == EVENT_KEY_DOWN)
	{
		if (e->Data.Keyboard.KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}

	if (e->Type == EVENT_FORM_INTERACTION && e->Data.Forms.EventFlag == FormEventType::ButtonClick)
	{
		if (e->Data.Forms.RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::POP;
			return;
		}
	}
}

void BuildingScreen::Update(StageCmd *const cmd)
{
	menuform->Update();
	*cmd = this->stageCmd;
	// Reset the command to default
	this->stageCmd = StageCmd();
}

void BuildingScreen::Render()
{
	fw.Stage_GetPrevious(this->shared_from_this())->Render();
	fw.renderer->drawFilledRect({0, 0}, fw.Display_GetSize(), Colour{0, 0, 0, 128});
	menuform->Render();
	fw.gamecore->MouseCursor->Render();
}

bool BuildingScreen::IsTransition() { return false; }

}; // namespace OpenApoc
