#include "game/ui/general/mainmenu.h"
#include "forms/ui.h"
#include "framework/event.h"
#include "framework/framework.h"
#include "game/ui/city/cityview.h"
#include "game/ui/debugtools/debugmenu.h"
#include "game/ui/general/difficultymenu.h"
#include "game/ui/general/loadingscreen.h"
#include "game/ui/general/optionsmenu.h"
#include "game/ui/general/savemenu.h"
#include "version.h"

namespace OpenApoc
{

static std::vector<UString> tracks{"music:0", "music:1", "music:2"};

MainMenu::MainMenu() : Stage(), mainmenuform(ui().getForm("FORM_MAINMENU"))
{
	auto versionLabel = mainmenuform->findControlTyped<Label>("VERSION_LABEL");
	versionLabel->setText(OPENAPOC_VERSION);
}

MainMenu::~MainMenu() = default;

void MainMenu::begin() { fw().jukebox->play(tracks); }

void MainMenu::pause() {}

void MainMenu::resume() {}

void MainMenu::finish() {}

void MainMenu::eventOccurred(Event *e)
{
	mainmenuform->eventOccured(e);

	if (e->type() == EVENT_KEY_DOWN)
	{
		if (e->keyboard().KeyCode == SDLK_ESCAPE)
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
	}

	if (e->type() == EVENT_FORM_INTERACTION && e->forms().EventFlag == FormEventType::ButtonClick)
	{
		if (e->forms().RaisedBy->Name == "BUTTON_OPTIONS")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<OptionsMenu>();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_QUIT")
		{
			stageCmd.cmd = StageCmd::Command::QUIT;
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_NEWGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<DifficultyMenu>();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_DEBUG")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<DebugMenu>();
			return;
		}
		if (e->forms().RaisedBy->Name == "BUTTON_LOADGAME")
		{
			stageCmd.cmd = StageCmd::Command::PUSH;
			stageCmd.nextStage = mksp<SaveMenu>(SaveMenuAction::LoadNewGame, nullptr);
			return;
		}
	}
}

void MainMenu::update(StageCmd *const cmd)
{
	mainmenuform->update();
	*cmd = stageCmd;
	stageCmd = StageCmd();
}

void MainMenu::render() { mainmenuform->render(); }

bool MainMenu::isTransition() { return false; }
}; // namespace OpenApoc
