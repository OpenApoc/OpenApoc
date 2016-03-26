#include "game/general/mainmenu.h"
#include "framework/framework.h"
#include "game/city/cityview.h"
#include "game/debugtools/debugmenu.h"
#include "game/general/difficultymenu.h"
#include "game/general/optionsmenu.h"
#include "game/resources/gamecore.h"
#include "version.h"

namespace OpenApoc
{

static std::vector<UString> tracks{"music:0", "music:1", "music:2"};

MainMenu::MainMenu() : Stage(), mainmenuform(fw().gamecore->GetForm("FORM_MAINMENU"))
{
	auto versionLabel = mainmenuform->FindControlTyped<Label>("VERSION_LABEL");
	versionLabel->SetText(OPENAPOC_VERSION);
}

MainMenu::~MainMenu() {}

void MainMenu::Begin() { fw().jukebox->play(tracks); }

void MainMenu::Pause() {}

void MainMenu::Resume() {}

void MainMenu::Finish() {}

void MainMenu::EventOccurred(Event *e)
{
	mainmenuform->EventOccured(e);
	fw().gamecore->MouseCursor->EventOccured(e);

	if (e->Type() == EVENT_KEY_DOWN)
	{
		if (e->Keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION && e->Forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->Forms().RaisedBy->Name == "BUTTON_OPTIONS")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<OptionsMenu>();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_NEWGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<DifficultyMenu>();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_DEBUG")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<DebugMenu>();
			return;
		}
		if (e->Forms().RaisedBy->Name == "BUTTON_LOADGAME")
		{
			// FIXME: Save game selector
			auto state = mksp<GameState>();
			if (state->loadGame("save") == false)
			{
				LogError("Failed to load 'save'");
				return;
			}
			state->initState();
			stageCmd.cmd = StageCmd::Command::PUSH;
			// FIXME:
			// if (state->inBattlescale)
			// 	nextStage = mksp<BattleScape>..
			// else
			stageCmd.nextStage =
			    mksp<CityView>(state, StateRef<City>{state.get(), "CITYMAP_HUMAN"});
		}
	}

	if (e->Type() == EVENT_FORM_INTERACTION &&
	    e->Forms().EventFlag == FormEventType::CheckBoxChange)
	{
		if (e->Forms().RaisedBy->Name == "CHECK_DEBUGMODE")
		{
			fw().gamecore->DebugModeEnabled =
			    std::dynamic_pointer_cast<CheckBox>(e->Forms().RaisedBy)->IsChecked();
			e->Forms().RaisedBy->GetForm()->FindControl("BUTTON_DEBUG")->Visible =
			    fw().gamecore->DebugModeEnabled;
		}
	}
}

void MainMenu::Update(StageCmd *const cmd)
{
	mainmenuform->Update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void MainMenu::Render()
{
	mainmenuform->Render();
	fw().gamecore->MouseCursor->Render();
}

bool MainMenu::IsTransition() { return false; }
}; // namespace OpenApoc
